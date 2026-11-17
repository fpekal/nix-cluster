#include "cluster.h"

#include <functional>
#include <future>
#include <thread>
#include <variant>
#include <vector>

#include <logger.h>
#include <util.h>
#include <uuid.h>

Cluster::Cluster(std::string_view url) : rabbitmq{url} {
  heartbeat_thread =
      std::jthread{std::bind_front<&Cluster::heartbeat_thread_handler>(this)};

  remove_dead_thread =
      std::jthread{std::bind_front<&Cluster::remove_dead_thread_handler>(this)};

  monitor_thread =
      std::jthread{std::bind_front<&Cluster::monitor_thread_handler>(this)};

  // Initialize all message handlers' containers
  auto message_types = get_all_enum_values<Message::MessageType>();
  for (auto type : message_types) {
    message_handlers[type] = {};
  }

  // Handler to save data about builders that have sent the `initialize_builder`
  // message
  bind_handler(Message::INITIALIZE_BUILDER,
               std::make_shared<Handler>(
                   [this](void *, const AllMessagesVariant &message) {
                     auto &m = std::get<MessageInitializeBuilder>(message);

                     std::lock_guard guard{builders_mutex};

                     builders.insert(
                         {m.sender,
                          {m.sender, m.content.name, m.content.client_config}});
                   }));

  // Run according handlers when message arrives
  rabbitmq.message_received_handler = [this](
                                          const AllMessagesVariant &message) {
    // Run all handlers of some type
    auto run_for_handlers = [](std::vector<std::shared_ptr<Handler>> &handlers,
                               const AllMessagesVariant &message) {
      auto handlers_copy = handlers;
      for (auto &handler : handlers_copy) {
        (*handler)(&handler, message);
      }
    };

    // Checks message type and runs `run_for_handlers` for that message type
    if (std::holds_alternative<MessageInitializeBuilder>(message)) {
      run_for_handlers(
          message_handlers.at(Message::MessageType::INITIALIZE_BUILDER),
          message);
    } else if (std::holds_alternative<MessagePong>(message)) {
      run_for_handlers(message_handlers.at(Message::MessageType::PONG),
                       message);
    }
  };
}

Cluster::~Cluster() {}

void Cluster::bind_handler(Message::MessageType type,
                           std::shared_ptr<Handler> handler) {
  std::lock_guard guard{message_handlers_mutex};
  auto &handlers = message_handlers.at(type);
  handlers.push_back(handler);
}

bool Cluster::unbind_handler(Message::MessageType type,
                             std::shared_ptr<Handler> handler) {
  std::lock_guard guard{message_handlers_mutex};
  auto &handlers = message_handlers.at(type);
  auto found = handlers.end();

  for (auto iter = handlers.begin(); iter != handlers.end(); ++iter) {
    if (*iter == handler) {
      found = iter;
    }
  }

  if (found != handlers.end()) {
    handlers.erase(found);
    return true;
  }

  return false;
}

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
      auto pong_handler_fn = std::make_shared<Handler>(
          [this, &promise,
           ping_message_uuid](void *self, const AllMessagesVariant &message) {
            auto this_handler =
                *reinterpret_cast<std::shared_ptr<Handler> *>(self);
            auto &m = std::get<MessagePong>(message);

            // If it's some other PONG message, ignore it
            if (m.content.ping_uuid != ping_message_uuid)
              return;

            // Tell the PONG was sent
            promise.set_value();

            // It was this builder.
            // Stop listening for more PONGs
            unbind_handler(Message::PONG, this_handler);
          });
      bind_handler(Message::PONG, pong_handler_fn);

      // Send message to the builder so it can respond to us
      rabbitmq.send_message_to_one(ping_message, uuid);

      // Wait for the PONG for TIMEOUT
      if (future.wait_for(std::chrono::seconds{5}) ==
          std::future_status::timeout) {
        // There was no PONG, stop listening and return failure
        // Builder is dead
        auto ret = unbind_handler(Message::PONG, pong_handler_fn);
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
        // TODO: return name instead of uuid
        Logger::get_logger().log(LoggerLevel::INFO, "Builder {} is dead",
                                 future.first);
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
