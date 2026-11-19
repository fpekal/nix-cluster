#include "cluster.h"

#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <stop_token>
#include <sys/stat.h>
#include <thread>
#include <variant>
#include <vector>

#include <logger.h>
#include <message.h>
#include <nlohmann/json.hpp>
#include <util.h>
#include <uuid.h>

Task::Task(std::shared_ptr<TaskSet> task_set, ServerRabbitMQ *rabbitmq,
           Cluster *cluster, std::string_view builder_uuid,
           std::string_view client_config, std::string_view task_input)
    : task_set{task_set}, rabbitmq{rabbitmq}, cluster{cluster},
      builder_uuid{builder_uuid}, client_config{client_config},
      task_input{task_input} {}

void Task::build() {

  MessageTaskRequest new_message;
  new_message.message_uuid = UUID::generate();
  new_message.sender = "server";

  new_message.content.flake_url = task_set->flake_url;
  new_message.content.client_config = client_config;
  new_message.content.task_input = task_input;

  auto task_completed_response_handler =
      std::make_shared<MessageDispatcher::Handler>();
  auto task_request_response_handler =
      std::make_shared<MessageDispatcher::Handler>();

  *task_request_response_handler =
      [new_message, this,
       task_request_response_handler =
           std::weak_ptr<MessageDispatcher::Handler>{
               task_request_response_handler},
       task_completed_response_handler =
           std::weak_ptr<MessageDispatcher::Handler>{
               task_completed_response_handler}](
          const AllMessagesVariant &message) {
        const MessageTaskRequestResponse &m =
            std::get<MessageTaskRequestResponse>(message);

        if (m.content.request_uuid != new_message.message_uuid)
          return;

        if (m.content.response == "success") {
          Logger::get_logger().log(INFO, "Builder {} accepted the {} task",
                                   cluster->get_builder_name(builder_uuid),
                                   new_message.content.flake_url);
        } else {
          Logger::get_logger().log(INFO, "Builder {} revoked the {} task",
                                   cluster->get_builder_name(builder_uuid),
                                   new_message.content.flake_url);
          cluster->message_dispatcher.unbind_handler(
              Message::TASK_COMPLETED, task_completed_response_handler.lock());
        }

        cluster->message_dispatcher.unbind_handler(
            Message::TASK_REQUEST_RESPONSE,
            task_request_response_handler.lock());
      };

  *task_completed_response_handler =
      [new_message, this,
       task_request_response_handler =
           std::weak_ptr<MessageDispatcher::Handler>{
               task_request_response_handler},
       task_completed_response_handler =
           std::weak_ptr<MessageDispatcher::Handler>{
               task_completed_response_handler}](
          const AllMessagesVariant &message) {
        const MessageTaskCompleted &m = std::get<MessageTaskCompleted>(message);

        if (m.content.request_uuid != new_message.message_uuid)
          return;

        if (m.content.response == "success") {
          Logger::get_logger().log(INFO, "Builder {} failed the {} task",
                                   cluster->get_builder_name(builder_uuid),
                                   new_message.content.flake_url);
        } else {
          Logger::get_logger().log(INFO, "Builder {} succeded the {} task",
                                   cluster->get_builder_name(builder_uuid),
                                   new_message.content.flake_url);
        }

        completed = true;

        cluster->message_dispatcher.unbind_handler(
            Message::TASK_COMPLETED, task_completed_response_handler.lock());
        cluster->message_dispatcher.unbind_handler(
            Message::TASK_REQUEST_RESPONSE,
            task_request_response_handler.lock());
      };

  cluster->message_dispatcher.bind_handler(Message::TASK_REQUEST_RESPONSE,
                                           task_request_response_handler);

  cluster->message_dispatcher.bind_handler(Message::TASK_COMPLETED,
                                           task_completed_response_handler);

  rabbitmq->send_message_to_one(new_message, builder_uuid);
}

bool Task::has_completed() { return completed; }

Generator::Generator(std::shared_ptr<TaskSet> task_set,
                     ServerRabbitMQ *rabbitmq, Cluster *cluster)
    : task_set{task_set}, rabbitmq{rabbitmq}, cluster{cluster} {
  auto temp_directory = std::filesystem::temp_directory_path();
  pipes_dir = temp_directory / std::string{UUID::generate()};

  std::filesystem::create_directory(pipes_dir);

  input_pipe = pipes_dir / "input";
  output_pipe = pipes_dir / "output";

  // Create named pipes
  mknod(std::string{input_pipe}.c_str(), S_IFIFO | S_IRUSR | S_IWUSR, 0);
  mknod(std::string{output_pipe}.c_str(), S_IFIFO | S_IRUSR | S_IWUSR, 0);

  // Response for PINGs
  message_dispatcher.bind_handler(
      Message::LOG, std::make_shared<MessageDispatcher::Handler>(
                        [this](const AllMessagesVariant &message) {
                          const MessageLog &m = std::get<MessageLog>(message);

                          Logger::get_logger().log_raw(
                              INFO, "[Generator {}] {}",
                              this->task_set->flake_url, m.content.message);
                        }));

  // Return all builders info when the generator asks for it
  message_dispatcher.bind_handler(
      Message::GET_BUILDERS,
      std::make_shared<MessageDispatcher::Handler>(
          [this](const AllMessagesVariant &message) {
            const MessageGetBuilders &m = std::get<MessageGetBuilders>(message);

            MessageGetBuildersResponse new_message;
            new_message.message_uuid = UUID::generate();
            new_message.sender = "server";
            new_message.content.request_uuid = m.message_uuid;

            for (auto builder : this->cluster->get_builders()) {
              new_message.content.builders.push_back(builder.second);
            }

            this->rabbitmq->send_message_to_one(new_message, m.sender);
          }));
}

Generator::~Generator() {
  std::filesystem::remove(output_pipe);
  std::filesystem::remove(input_pipe);

  std::filesystem::remove(pipes_dir);

  if (generator_stream != nullptr) {
    pclose(generator_stream);
  }
}

void Generator::run() {
  IPC_thread =
      std::jthread(std::bind_front<&Generator::IPC_thread_handler>(this));

  std::string formatted = std::format(
      "nix run \"{}#generator\" -- \"{}\" \"{}\"", task_set->flake_url,
      std::string{input_pipe}, std::string{output_pipe});
  Logger::get_logger().log(DEBUG, "Running generator with: `{}`", formatted);

  generator_stream = popen(formatted.c_str(), "r");
}

void Generator::IPC_thread_handler(std::stop_token stop_token) {
  std::ifstream input{input_pipe};

  try {
    while (!stop_token.stop_requested()) {
      nlohmann::json json;
      input >> json;

      auto message = json_to_message(json.dump());

      message_dispatcher.dispatch(message.value());
    }
  } catch (nlohmann::json::exception &e) {
    // `input` returned empty string
    // so the generator has died
    // Or generator returned invalid json
    //
    // Is this exception a bug? I don't think so?
    // It works... and it's not a likely branch of the program
    //
    // When such error occurs, stop listening to more messages.
  }
}

Cluster::Cluster(std::string_view url) : rabbitmq{url} {
  auto task_set = std::make_shared<TaskSet>();
  task_set->flake_url = ".";
  Generator g{task_set, &rabbitmq, this};
  g.run();

  heartbeat_thread =
      std::jthread{std::bind_front<&Cluster::heartbeat_thread_handler>(this)};

  remove_dead_thread =
      std::jthread{std::bind_front<&Cluster::remove_dead_thread_handler>(this)};

  monitor_thread =
      std::jthread{std::bind_front<&Cluster::monitor_thread_handler>(this)};

  discovery_thread =
      std::jthread{std::bind_front<&Cluster::discovery_thread_handler>(this)};

  // Handler to save data about builders that have sent the `initialize_builder`
  // message
  message_dispatcher.bind_handler(
      Message::INITIALIZE_BUILDER,
      std::make_shared<MessageDispatcher::Handler>(
          [this](const AllMessagesVariant &message) {
            auto &m = std::get<MessageInitializeBuilder>(message);

            Logger::get_logger().log(INFO, "Builder {} connected",
                                     m.content.name);

            std::lock_guard guard{builders_mutex};

            builders.insert(
                {m.sender,
                 {m.sender, m.content.name, m.content.client_config}});
          }));

  // Handler to save data about builders that have sent the `discovery_response`
  // message
  // Saves status of the builder
  message_dispatcher.bind_handler(
      Message::DISCOVERY_RESPONSE,
      std::make_shared<MessageDispatcher::Handler>(
          [this](const AllMessagesVariant &message) {
            auto &m = std::get<MessageDiscoveryResponse>(message);

            std::lock_guard guard{builders_mutex};

            auto found = builders.find(m.sender);
            if (found == builders.end()) {
              Logger::get_logger().log(INFO, "Builder {} connected",
                                       m.content.name);
              builders.insert({m.sender,
                               {m.sender, m.content.name,
                                m.content.client_config, m.content.status}});
            } else {
              found->second.status = m.content.status;
            }
          }));

  // Respond to PINGs with PONGs
  message_dispatcher.bind_handler(
      Message::PING, std::make_shared<MessageDispatcher::Handler>(
                         [this](const AllMessagesVariant &message) {
                           auto &m = std::get<MessagePing>(message);

                           MessagePong new_message;
                           new_message.sender = "server";
                           new_message.message_uuid = UUID::generate();
                           new_message.content.ping_uuid = m.message_uuid;
                           new_message.content.timestamp =
                               std::chrono::high_resolution_clock::now();

                           rabbitmq.send_message_to_one(new_message, m.sender);
                         }));

  // Put into local logs all logs sent by builders
  message_dispatcher.bind_handler(
      Message::LOG, std::make_shared<MessageDispatcher::Handler>(
                        [this](const AllMessagesVariant &message) {
                          auto &m = std::get<MessageLog>(message);

                          std::string builder_name = get_builder_name(m.sender);

                          Logger::get_logger().log_raw(
                              LoggerLevel::INFO, "[\e[0;33mBuilder {}\e[0m] {}",
                              builder_name, m.content.message);
                        }));

  // Run according handlers when message arrives
  rabbitmq.message_received_handler =
      [this](const AllMessagesVariant &message) {
        message_dispatcher.dispatch(message);
      };
}

Cluster::~Cluster() {}

void Cluster::heartbeat_thread_handler(std::stop_token stop_token) {
  // Wait for rabbitmq to complete connection
  rabbitmq.get_ready_future().wait();

  while (!stop_token.stop_requested()) {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(5s);

    rabbitmq.heartbeat();
  }
}

Builders Cluster::get_builders() {
  std::lock_guard guard{builders_mutex};
  return builders;
}

std::string Cluster::get_builder_name(std::string_view uuid) {
  std::string builder_name = "";
  {
    std::lock_guard guard{builders_mutex};
    auto found = builders.find(std::string{uuid});

    if (found == builders.end()) {
      builder_name = uuid;
    } else {
      builder_name = found->second.name;
    }
  }

  return builder_name;
}

void Cluster::remove_dead_thread_handler(std::stop_token stop_token) {
  // Wait for rabbitmq to complete connection
  rabbitmq.get_ready_future().wait();

  while (!stop_token.stop_requested()) {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(10s);

    // Copy whole map so the rest of the function can be done asynchronously.
    std::unique_lock lock{builders_mutex};
    Builders builders = this->builders;
    lock.unlock();

    // Is builder responding to pings?
    auto is_responding = [this](std::string uuid) {
      MessagePing ping_message;
      auto ping_message_uuid = std::string{UUID::generate()};
      ping_message.message_uuid = ping_message_uuid;
      ping_message.sender = "server";

      // Message handler will fire on some other thread and the result will be
      // stored in this promise and future
      // If the promise will fullfill in TIMEOUT, then the builder has responded
      // auto promise = std::make_shared<std::promise<void>>();
      std::promise<void> promise;
      std::future future = promise.get_future();

      // Listen to the PONG message
      std::shared_ptr<MessageDispatcher::Handler> pong_handler_fn =
          std::make_shared<MessageDispatcher::Handler>();

      *pong_handler_fn =
          [this, &promise, ping_message_uuid,
           pong_handler_fn = std::weak_ptr<MessageDispatcher::Handler>{
               pong_handler_fn}](const AllMessagesVariant &message) {
            auto &m = std::get<MessagePong>(message);

            // If it's some other PONG message, ignore it
            if (m.content.ping_uuid != ping_message_uuid)
              return;

            // Tell the PONG was sent
            promise.set_value();

            // It was this builder.
            // Stop listening for more PONGs
            message_dispatcher.unbind_handler(Message::PONG,
                                              pong_handler_fn.lock());
          };
      message_dispatcher.bind_handler(Message::PONG, pong_handler_fn);

      // Send message to the builder so it can respond to us
      rabbitmq.send_message_to_one(ping_message, uuid);

      // Wait for the PONG for TIMEOUT
      if (future.wait_for(std::chrono::seconds{5}) ==
          std::future_status::timeout) {
        // There was no PONG, stop listening and return failure
        // Builder is dead
        auto ret =
            message_dispatcher.unbind_handler(Message::PONG, pong_handler_fn);
        return false;
      }

      // There was PONG!
      // Builder is alive
      return true;
    };

    // Start all checks and save their futures
    std::map<std::string, std::future<bool>> futures;
    for (auto &builder : builders) {
      // Ping builder on the separate thread
      auto future = std::async(std::bind_back(is_responding, builder.first));

      futures.insert({builder.first, std::move(future)});
    }

    // UUIDs of builder that didn't respond to the PING
    std::vector<std::string> uuids_to_delete;

    for (auto &future : futures) {
      // Wait and get the result of pinging
      if (!future.second.get()) {
        std::string builder_name = builders.at(future.first).name;

        Logger::get_logger().log(LoggerLevel::INFO, "Builder {} is dead",
                                 builder_name);
        // The builder didn't respond. Let's delete them from the memory.
        uuids_to_delete.push_back(future.first);
      }
    }

    // Working on the real `builder`
    lock.lock();
    for (auto &uuid : uuids_to_delete) {
      this->builders.erase(uuid);
    }
    lock.unlock();
  }
}

void Cluster::monitor_thread_handler(std::stop_token stop_token) {
  // DON'T wait for rabbitmq connection, as monitoring sockets is required to
  // complete this connection.

  while (!stop_token.stop_requested()) {
    rabbitmq.monitor(1000);
  }
}

void Cluster::discovery_thread_handler(std::stop_token stop_token) {
  // Wait for rabbitmq to complete connection
  rabbitmq.get_ready_future().wait();

  while (!stop_token.stop_requested()) {
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(10s);

    MessageDiscovery message;
    message.sender = "server";
    message.message_uuid = UUID::generate();

    rabbitmq.send_message_broadcast(message);
  }
}
