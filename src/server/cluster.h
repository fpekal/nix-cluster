#pragma once
#include <functional>
#include <memory>
#include <stop_token>
#include <thread>

#include <message.h>
#include <rabbitmq.h>

#include "builder.h"

class Cluster {
public:
  typedef std::function<void(void *, const AllMessagesVariant &)> Handler;

  Cluster(std::string_view url);
  ~Cluster();

  // All handlers are run when some kind of message is received.
  void bind_handler(Message::MessageType type,
                    std::shared_ptr<Handler> handler);

  // `handler` is the value returned by the `bind_handler()` function.
  // Returns true if the handler was found and unbound.
  bool unbind_handler(Message::MessageType type,
                      std::shared_ptr<Handler> handler);

private:
  void heartbeat_thread_handler(std::stop_token stop_token);
  void remove_dead_thread_handler(std::stop_token stop_token);
  void monitor_thread_handler(std::stop_token stop_token);
  void discovery_thread_handler(std::stop_token stop_token);

  Builders builders;
  std::mutex builders_mutex;

  ServerRabbitMQ rabbitmq;

  // When rabbitmq receives a message, it gets handled by ALL handlers of this
  // type of the message.
  std::map<Message::MessageType, std::vector<std::shared_ptr<Handler>>>
      message_handlers;
  std::mutex message_handlers_mutex;

  // Sends heartbeat to the broker once every 5 seconds
  std::jthread heartbeat_thread;
  // Pings every builder in the `builders` and checks if they respond.
  // If not, delete them from `builders`.
  std::jthread remove_dead_thread;
  // Checks if sockets are ready for writing and reading and reports their
  // status to the rabbitmq
  std::jthread monitor_thread;
  // Sends `discovery` message to all builders.
  // This allows for discovery of new builders and getting their status.
  std::jthread discovery_thread;
};
