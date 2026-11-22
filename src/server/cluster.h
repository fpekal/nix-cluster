#pragma once
#include <condition_variable>
#include <queue>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <stop_token>
#include <thread>

#include <builder.h>
#include <message.h>
#include <rabbitmq.h>

class Cluster;

// Basically what clients send to the server.
// Contains url to the flake with task generator and task template
class TaskSet {
public:
  std::string flake_url;
};

// Task on the builder
class Task {
public:
  Task(std::shared_ptr<TaskSet> task_set, ServerRabbitMQ *rabbitmq,
       Cluster *cluster, std::string_view builder_uuid,
       std::string_view client_config, std::string_view task_input);

  std::shared_ptr<TaskSet> task_set;

  void build();
  bool has_completed();

private:
  std::string client_config;
  std::string task_input;
  std::string builder_uuid;
  ServerRabbitMQ *rabbitmq;
  Cluster *cluster;

  std::atomic<bool> completed = false;
};

// Runs the generator and creates tasks
class Generator {
public:
  Generator(std::shared_ptr<TaskSet> task_set, ServerRabbitMQ *rabbitmq,
            Cluster *cluster);
  ~Generator();

  void run();

private:
  void IPC_input_thread_handler(std::stop_token stop_token);
  void IPC_output_thread_handler(std::stop_token stop_token);

  MessageDispatcher message_dispatcher;
  std::vector<Task> tasks;

  std::shared_ptr<TaskSet> task_set;
  ServerRabbitMQ *rabbitmq;
  Cluster *cluster;

  std::filesystem::path pipes_dir;
  std::filesystem::path input_pipe;
  std::filesystem::path output_pipe;

  std::queue<AllMessagesVariant> output_queue;
  std::mutex output_queue_mutex;
  std::condition_variable output_queue_cv;
  void send_message(const AllMessagesVariant& message);

  std::jthread IPC_input_thread;
  std::jthread IPC_output_thread;
  FILE *generator_stream = nullptr;
};

// Main class
// Tracks builders, tasks on them, etc.
class Cluster {
public:
  Cluster(std::string_view url);
  ~Cluster();

  Builders get_builders();
  std::string get_builder_name(std::string_view uuid);

  MessageDispatcher message_dispatcher;

private:
  void heartbeat_thread_handler(std::stop_token stop_token);
  void remove_dead_thread_handler(std::stop_token stop_token);
  void monitor_thread_handler(std::stop_token stop_token);
  void discovery_thread_handler(std::stop_token stop_token);

  Builders builders;
  std::mutex builders_mutex;

  ServerRabbitMQ rabbitmq;

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
