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
  Cluster(std::string_view url);
  ~Cluster();

private:
  void heartbeat_thread_handler(std::stop_token stop_token);
  void remove_dead_thread_handler(std::stop_token stop_token);
  void monitor_thread_handler(std::stop_token stop_token);
  void discovery_thread_handler(std::stop_token stop_token);

  Builders builders;
  std::mutex builders_mutex;

  ServerRabbitMQ rabbitmq;

  MessageDispatcher message_dispatcher;

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
