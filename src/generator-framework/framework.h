#pragma once
#include <condition_variable>
#include <filesystem>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

#include <builder.h>
#include <message.h>

#include "task.h"

// Handles all communication with a server and clients.
// Create an object using `Framework::create()`.
class Framework {
  friend class Task;
  friend class LoggerSinkRemote;

public:
  static std::shared_ptr<Framework> create(int argc, char *argv[]);

  // Get all registered builders.
  std::future<Builders> get_builders();
  // Create and send some task to the builder.
  // `flake_url` is {this}#task part of the flake
  // `client_config` is a nix expression that will be sent to the nix task
  // definition. Put there some flags that are specific to the client. For
  // example `-march=` or `use_gpu` etc. `task_input` is also sent to the nix
  // task definition.
  //
  // Both these things don't mean anything "specific". You decide what to do
  // with them in the task definition.
  Task create_task(std::string_view builder_uuid,
                   std::string_view client_config, std::string_view flake_url,
                   std::string_view task_input);

private:
  // Create Framework object using `Frameworl::create()`
  Framework(int argc, char *argv[]);

  void input_thread_handler(std::stop_token stop_token);
  void output_thread_handler(std::stop_token stop_token);

  MessageDispatcher message_dispatcher;

  std::queue<AllMessagesVariant> output_queue;
  std::mutex output_queue_mutex;
  std::condition_variable output_queue_cv;
  void send_message(const AllMessagesVariant &message);

  std::filesystem::path input_pipe;
  std::filesystem::path output_pipe;

  std::jthread input_thread;
  std::jthread output_thread;

  std::weak_ptr<Framework> self;
};
