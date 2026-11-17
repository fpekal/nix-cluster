#include "remote_sink.h"

#include <print>

#include <uuid.h>

void LoggerSinkRemote::send_raw_message(std::string_view text) {
  std::println("{}", text);

  if (!rabbitmq.is_ready()) {
    queue.push(std::string{text});
    return;
  }

  while (!queue.empty()) {
    std::string queued_text = queue.front();
    queue.pop();

    MessageLog message;
    message.sender = client_uuid;
    message.message_uuid = UUID::generate();
    message.content.message = queued_text;

    rabbitmq.send_message(message);
  }

  MessageLog message;
  message.sender = client_uuid;
  message.message_uuid = UUID::generate();
  message.content.message = text;

  rabbitmq.send_message(message);
}

void LoggerSinkRemote::flush() {}
