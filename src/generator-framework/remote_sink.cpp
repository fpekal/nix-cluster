#include "remote_sink.h"

#include <print>

#include <uuid.h>

void LoggerSinkRemote::send_raw_message(std::string_view text) {
  std::println("{}", text);

  MessageLog message;
  message.sender = "generator";
  message.message_uuid = UUID::generate();
  message.content.message = text;

  framework.send_message(message);
}

void LoggerSinkRemote::flush() {}
