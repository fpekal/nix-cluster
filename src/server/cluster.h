#pragma once
#include <filesystem>
#include <functional>
#include <memory>
#include <stop_token>
#include <thread>

#include <builder.h>
#include <message.h>
#include <rabbitmq.h>

// Basically what clients send to the server.
// Contains url to the flake with task generator and task template
class TaskSet {
public:
  std::string flake_url;
};

// Task on the builder
class Task {
public:
  std::shared_ptr<TaskSet> task_set;

  void build();
  bool has_completed();
};

// Runs the generator and creates tasks
class Generator {
public:
  Generator(std::shared_ptr<TaskSet> task_set, RabbitMQ *rabbitmq);
  ~Generator();

  void run();

private:
  void IPC_thread_handler(std::stop_token stop_token);

  MessageDispatcher message_dispatcher;

  std::shared_ptr<TaskSet> task_set;
  RabbitMQ *rabbitmq;

  std::filesystem::path pipes_dir;
  std::filesystem::path input_pipe;
  std::filesystem::path output_pipe;

  std::jthread IPC_thread;
  FILE *generator_stream = nullptr;
};

// Main class
// Tracks builders, tasks on them, etc.
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
