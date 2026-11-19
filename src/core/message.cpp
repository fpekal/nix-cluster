#include "message.h"

#include <algorithm>
#include <chrono>
#include <string>

#include <nlohmann/json.hpp>

#include "util.h"

std::string Message::message_type_to_str(MessageType mes_type) {
  static constexpr auto conv_array =
      generate_enum_to_string_view_array<MessageType>();

  for (int i = 0; i < conv_array.size(); ++i) {
    if (mes_type == conv_array[i].first) {
      std::string s = std::string{conv_array[i].second};
      std::transform(s.begin(), s.end(), s.begin(),
                     [](unsigned char c) { return std::tolower(c); });
      return s;
    }
  }

  return "";
}

std::string Message::builder_status_to_str(BuilderStatus builder_status) {
  static constexpr auto conv_array =
      generate_enum_to_string_view_array<BuilderStatus>();

  for (int i = 0; i < conv_array.size(); ++i) {
    if (builder_status == conv_array[i].first) {
      std::string s = std::string{conv_array[i].second};
      std::transform(s.begin(), s.end(), s.begin(),
                     [](unsigned char c) { return std::tolower(c); });
      return s;
    }
  }

  return "";
}

Message::BuilderStatus Message::str_to_builder_status(std::string_view str) {
  static constexpr auto conv_array =
      generate_enum_to_string_view_array<BuilderStatus>();

  for (int i = 0; i < conv_array.size(); ++i) {
    std::string str_up = std::string{str};
    std::transform(str_up.begin(), str_up.end(), str_up.begin(),
                   [](unsigned char c) { return std::toupper(c); });
    if (str == conv_array[i].second) {
      return conv_array[i].first;
    }
  }

  return IDLE;
}

std::string message_to_json(const Message &mes) {
  nlohmann::json json{};

  json["message_uuid"] = mes.message_uuid;
  json["type"] = Message::message_type_to_str(mes.type);
  json["sender"] = mes.sender;

  switch (mes.type) {
  case Message::INITIALIZE_BUILDER: {
    auto &m = static_cast<const MessageInitializeBuilder &>(mes);
    json["content"]["name"] = m.content.name;
    json["content"]["client_config"] = m.content.client_config;
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
  case Message::DISCOVERY:
    break;
  case Message::DISCOVERY_RESPONSE: {
    auto &m = static_cast<const MessageDiscoveryResponse &>(mes);
    json["content"]["request_uuid"] = m.content.request_uuid;
    json["content"]["name"] = m.content.name;
    json["content"]["client_config"] = m.content.client_config;
    json["content"]["status"] =
        Message::builder_status_to_str(m.content.status);
    break;
  }
  }

  return json.dump();
}

std::optional<AllMessagesVariant> json_to_message(std::string_view json) {
  try {
    auto obj = nlohmann::json::parse(json);

    static auto fill_common = [](Message &mes, nlohmann::json &obj) {
      mes.message_uuid = obj["message_uuid"];
      mes.sender = obj["sender"];
    };

    if (obj["type"] == "initialize_builder") {
      MessageInitializeBuilder mes;
      fill_common(mes, obj);
      mes.content.client_config = obj["content"]["client_config"];
      mes.content.name = obj["content"]["name"];
      return AllMessagesVariant{mes};
    }
    if (obj["type"] == "task_request") {
      MessageTaskRequest mes;
      fill_common(mes, obj);
      mes.content.client_config = obj["content"]["client_config"];
      mes.content.flake_url = obj["content"]["flake_url"];
      mes.content.task_input = obj["content"]["task_input"];
      return AllMessagesVariant{mes};
    }
    if (obj["type"] == "task_request_response") {
      MessageTaskRequestResponse mes;
      fill_common(mes, obj);
      mes.content.response = obj["content"]["response"];
      mes.content.request_uuid = obj["content"]["request_uuid"];
      return AllMessagesVariant{mes};
    }
    if (obj["type"] == "task_completed") {
      MessageTaskCompleted mes;
      fill_common(mes, obj);
      mes.content.response = obj["content"]["response"];
      mes.content.request_uuid = obj["content"]["request_uuid"];
      mes.content.result = obj["content"]["result"];
      return AllMessagesVariant{mes};
    }
    if (obj["type"] == "log") {
      MessageLog mes;
      fill_common(mes, obj);
      mes.content.message = obj["content"]["message"];
      return AllMessagesVariant{mes};
    }
    if (obj["type"] == "ping") {
      MessagePing mes;
      fill_common(mes, obj);
      return AllMessagesVariant{mes};
    }
    if (obj["type"] == "pong") {
      MessagePong mes;
      fill_common(mes, obj);
      mes.content.ping_uuid = obj["content"]["ping_uuid"];
      mes.content.timestamp = std::chrono::high_resolution_clock::time_point{
          std::chrono::milliseconds{
              static_cast<uint64_t>(obj["content"]["timestamp"])}};
      return AllMessagesVariant{mes};
    }
    if (obj["type"] == "discovery") {
      MessageDiscovery mes;
      fill_common(mes, obj);
      return AllMessagesVariant{mes};
    }
    if (obj["type"] == "discovery_response") {
      MessageDiscoveryResponse mes;
      fill_common(mes, obj);
      mes.content.client_config = obj["content"]["client_config"];
      mes.content.request_uuid = obj["content"]["request_uuid"];
      mes.content.name = obj["content"]["name"];
      mes.content.status =
          Message::str_to_builder_status(obj["content"]["status"].dump());
      return AllMessagesVariant{mes};
    }

    return {};
  } catch (nlohmann::json::exception &e) {
    return {};
  }
}

MessageDispatcher::MessageDispatcher() {
  // Initialize all message handlers' containers
  auto message_types = get_all_enum_values<Message::MessageType>();
  for (auto type : message_types) {
    message_handlers[type] = {};
  }
}

void MessageDispatcher::bind_handler(Message::MessageType type,
                                     std::shared_ptr<Handler> handler) {
  std::lock_guard guard{message_handlers_mutex};
  auto &handlers = message_handlers.at(type);
  handlers.push_back(handler);
}

bool MessageDispatcher::unbind_handler(Message::MessageType type,
                                       std::shared_ptr<Handler> handler) {
  std::lock_guard guard{message_handlers_mutex};
  auto &handlers = message_handlers.at(type);
  auto found = handlers.end();

  for (auto iter = handlers.begin(); iter != handlers.end(); ++iter) {
    if (*iter == handler) {
      found = iter;
    }
  }

  if (found != handlers.end()) {
    handlers.erase(found);
    return true;
  }

  return false;
}

void MessageDispatcher::dispatch(const AllMessagesVariant &message) {
  // Run all handlers of some type
  auto run_for_handlers = [](std::vector<std::shared_ptr<Handler>> &handlers,
                             const AllMessagesVariant &message) {
    auto handlers_copy = handlers;
    for (auto &handler : handlers_copy) {
      (*handler)(&handler, message);
    }
  };

  // Checks message type and runs `run_for_handlers` for that message type
  if (std::holds_alternative<MessageInitializeBuilder>(message)) {
    run_for_handlers(message_handlers.at(Message::INITIALIZE_BUILDER), message);
  } else if (std::holds_alternative<MessageTaskRequest>(message)) {
    run_for_handlers(message_handlers.at(Message::TASK_REQUEST), message);
  } else if (std::holds_alternative<MessageTaskRequestResponse>(message)) {
    run_for_handlers(message_handlers.at(Message::TASK_REQUEST_RESPONSE),
                     message);
  } else if (std::holds_alternative<MessageTaskCompleted>(message)) {
    run_for_handlers(message_handlers.at(Message::TASK_COMPLETED), message);
  } else if (std::holds_alternative<MessageLog>(message)) {
    run_for_handlers(message_handlers.at(Message::LOG), message);
  } else if (std::holds_alternative<MessagePing>(message)) {
    run_for_handlers(message_handlers.at(Message::PING), message);
  } else if (std::holds_alternative<MessagePong>(message)) {
    run_for_handlers(message_handlers.at(Message::PONG), message);
  } else if (std::holds_alternative<MessageDiscovery>(message)) {
    run_for_handlers(message_handlers.at(Message::DISCOVERY), message);
  } else if (std::holds_alternative<MessageDiscoveryResponse>(message)) {
    run_for_handlers(message_handlers.at(Message::DISCOVERY_RESPONSE), message);
  }
}
