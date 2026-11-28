#pragma once
#include <future>
#include <memory>

#include <message.h>

class Framework;

// Running task on some builder
class Task {
  friend class Framework;

public:
  Task(Task &&other) = default;
  ~Task();

  // Future that will fullfill when the task is completed.
  std::shared_future<std::string> get_future() { return future; }

private:
  // You can create tasks only using `Framework`
  Task(std::shared_ptr<Framework> framework, std::string_view builder_uuid,
       std::string_view client_config, std::string_view flake_url,
       std::string_view task_input);

  std::shared_future<std::string> future;
  std::shared_ptr<Framework> framework;

  std::shared_ptr<MessageDispatcher::Handler>
      generator_task_request_response_fn =
          std::make_shared<MessageDispatcher::Handler>();
  std::shared_ptr<MessageDispatcher::Handler> generator_task_completed_fn =
      std::make_shared<MessageDispatcher::Handler>();
};
