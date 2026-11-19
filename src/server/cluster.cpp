#include "cluster.h"

#include <filesystem>
#include <fstream>
#include <functional>
#include <future>
#include <memory>
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

Generator::Generator(std::shared_ptr<TaskSet> task_set, RabbitMQ *rabbitmq)
    : task_set{task_set}, rabbitmq{rabbitmq} {
  auto temp_directory = std::filesystem::temp_directory_path();
  pipes_dir = temp_directory / std::string{UUID::generate()};

  std::filesystem::create_directory(pipes_dir);

  input_pipe = pipes_dir / "input";
  output_pipe = pipes_dir / "output";

  // Create named pipes
  mknod(std::string{input_pipe}.c_str(), S_IFIFO | S_IRUSR | S_IWUSR, 0);
  mknod(std::string{output_pipe}.c_str(), S_IFIFO | S_IRUSR | S_IWUSR, 0);

  message_dispatcher.bind_handler(
      Message::LOG, std::make_shared<MessageDispatcher::Handler>(
                        [](void *, const AllMessagesVariant &message) {
                          const MessageLog &m = std::get<MessageLog>(message);

                          Logger::get_logger().log_raw(
                              INFO, "[Generator Joshua] {}", m.content.message);
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

  generator_stream = popen(
      std::format("nix run {}#generator -- \"{}\" \"{}\"", task_set->flake_url,
                  std::string{input_pipe}, std::string{output_pipe})
          .c_str(),
      "r");
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
  Generator g{task_set, &rabbitmq};
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
          [this](void *, const AllMessagesVariant &message) {
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
          [this](void *, const AllMessagesVariant &message) {
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
                         [this](void *, const AllMessagesVariant &message) {
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
                        [this](void *, const AllMessagesVariant &message) {
                          auto &m = std::get<MessageLog>(message);

                          std::string builder_name = "";
                          {
                            auto found = builders.find(m.sender);
                            if (found == builders.end()) {
                              builder_name = m.sender;
                            } else {
                              builder_name = found->second.name;
                            }
                          }

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
      auto pong_handler_fn = std::make_shared<MessageDispatcher::Handler>(
          [this, &promise,
           ping_message_uuid](void *self, const AllMessagesVariant &message) {
            auto this_handler = *reinterpret_cast<
                std::shared_ptr<MessageDispatcher::Handler> *>(self);
            auto &m = std::get<MessagePong>(message);

            // If it's some other PONG message, ignore it
            if (m.content.ping_uuid != ping_message_uuid)
              return;

            // Tell the PONG was sent
            promise.set_value();

            // It was this builder.
            // Stop listening for more PONGs
            message_dispatcher.unbind_handler(Message::PONG, this_handler);
          });
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
