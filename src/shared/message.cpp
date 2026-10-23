#include "message.h"

#include <nlohmann/json.hpp>

std::string Message::message_type_to_str(MessageType mes_type) {
  switch (mes_type) {
  case INITIALIZE_BUILDER:
    return "initizlize_builder";
  case INITIALIZE_BUILDER_RESPONSE:
    return "initizlize_builder_response";
  case TASK_REQUEST:
    return "task_request";
  case TASK_REQUEST_RESPONSE:
    return "task_request_response";
  case TASK_COMPLETED:
    return "task_completed";
  case LOG:
    return "log";
  case PING:
    return "ping";
  case PONG:
    return "pong";
  }

  return "UNKNOWN";
}

std::string message_to_json(const Message &mes) {
  nlohmann::json json{};

  json["message_uuid"] = mes.message_uuid;
  json["type"] = Message::message_type_to_str(mes.type);

  switch (mes.type) {
  case Message::INITIALIZE_BUILDER: {
    auto &m = static_cast<const MessageInitializeBuilder &>(mes);
    json["content"]["name"] = m.content.name;
    json["content"]["client_config"] = m.content.client_config;
    break;
  }
  case Message::INITIALIZE_BUILDER_RESPONSE: {
    auto &m = static_cast<const MessageInitializeBuilderResponse &>(mes);
    json["content"]["response_uuid"] = m.content.response_uuid;
    json["content"]["response"] = m.content.response;
    break;
  }
  case Message::TASK_REQUEST: {
    auto &m = static_cast<const MessageTaskRequest &>(mes);
    json["content"]["flake_url"] = m.content.flake_url;
    json["content"]["client_config"] = m.content.client_config;
    json["content"]["task_input"] = m.content.task_input;
    break;
  }
  case Message::TASK_REQUEST_RESPONSE: {
    auto &m = static_cast<const MessageTaskRequestResponse &>(mes);
    json["content"]["request_uuid"] = m.content.request_uuid;
    json["content"]["response"] = m.content.response;
    break;
  }
  case Message::TASK_COMPLETED: {
    auto &m = static_cast<const MessageTaskCompleted &>(mes);
    json["content"]["request_uuid"] = m.content.request_uuid;
    json["content"]["response"] = m.content.response;
    json["content"]["result"] = m.content.result;
    break;
  }
  case Message::LOG: {
    auto &m = static_cast<const MessageLog &>(mes);
    json["content"]["message"] = m.content.message;
    break;
  }
  case Message::PING:
    break;
  case Message::PONG: {
    auto &m = static_cast<const MessagePong &>(mes);
    json["content"]["ping_uuid"] = m.content.ping_uuid;
    json["content"]["timestamp"] =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            m.content.timestamp.time_since_epoch())
            .count();
    break;
  }
  }

  return json.dump();
}
