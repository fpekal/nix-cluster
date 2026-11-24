#include "task.h"

#include <exception>

#include <message.h>
#include <uuid.h>

#include "framework.h"

Task::Task(std::shared_ptr<Framework> framework, std::string_view builder_uuid,
           std::string_view client_config, std::string_view flake_url,
           std::string_view task_input) {
  MessageGeneratorTaskRequest new_message;
  new_message.message_uuid = UUID::generate();
  new_message.sender = "generator";

  new_message.content.builder_uuid = builder_uuid;
  new_message.content.flake_url = flake_url;
  new_message.content.client_config = client_config;
  new_message.content.task_input = task_input;

  auto promise = std::make_shared<std::promise<std::string>>();
  future = promise->get_future().share();

  *generator_task_request_response_fn =
      [this, promise, new_message](const AllMessagesVariant &message) {
        const auto &m = std::get<MessageGeneratorTaskRequestResponse>(message);

        if (m.content.request_uuid != new_message.message_uuid)
          return;

        if (m.content.response != "success") {
          this->framework->message_dispatcher.unbind_handler(
              Message::GENERATOR_TASK_REQUEST_RESPONSE,
              generator_task_request_response_fn);
          this->framework->message_dispatcher.unbind_handler(
              Message::GENERATOR_TASK_COMPLETED, generator_task_completed_fn);

          promise->set_exception(std::make_exception_ptr(std::exception{}));
        }
      };

  *generator_task_completed_fn =
      [this, promise, new_message](const AllMessagesVariant &message) {
        const auto &m = std::get<MessageGeneratorTaskCompleted>(message);

        if (m.content.request_uuid != new_message.message_uuid)
          return;

        if (m.content.response != "success") {
          this->framework->message_dispatcher.unbind_handler(
              Message::GENERATOR_TASK_REQUEST_RESPONSE,
              generator_task_request_response_fn);
          this->framework->message_dispatcher.unbind_handler(
              Message::GENERATOR_TASK_COMPLETED, generator_task_completed_fn);

          promise->set_exception(std::make_exception_ptr(std::exception{}));
        }

        promise->set_value(m.content.result);
      };

  framework->message_dispatcher.bind_handler(
      Message::GENERATOR_TASK_REQUEST_RESPONSE,
      generator_task_request_response_fn);
  framework->message_dispatcher.bind_handler(Message::GENERATOR_TASK_COMPLETED,
                                             generator_task_completed_fn);

  framework->send_message(new_message);
}

Task::~Task() {
  if (framework && generator_task_request_response_fn &&
      generator_task_completed_fn) {
    framework->message_dispatcher.unbind_handler(
        Message::GENERATOR_TASK_REQUEST_RESPONSE,
        generator_task_request_response_fn);
    framework->message_dispatcher.unbind_handler(
        Message::GENERATOR_TASK_COMPLETED, generator_task_completed_fn);
  }
}
