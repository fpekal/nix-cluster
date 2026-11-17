#include "remote_sink.h"

#include <uuid.h>

void LoggerSinkRemote::send_raw_message(std::string_view text) {
  if (!rabbitmq.is_ready())
    return;

  MessageLog message;
  message.sender = client_uuid;
  message.message_uuid = UUID::generate();
  message.content.message = text;

  rabbitmq.send_message(message);
}

void LoggerSinkRemote::flush() {}
