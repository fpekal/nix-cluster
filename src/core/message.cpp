#include "message.h"

#include <algorithm>
#include <chrono>
#include <ranges>
#include <string>

#include <nlohmann/json.hpp>
#include <variant>

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

BuilderStatus Message::str_to_builder_status(std::string_view str) {
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
  case Message::GET_BUILDERS:
    break;
  case Message::GET_BUILDERS_RESPONSE: {
    auto &m = static_cast<const MessageGetBuildersResponse &>(mes);
    json["content"]["request_uuid"] = m.content.request_uuid;
    for (const auto &builder : m.content.builders) {
      nlohmann::json builder_json;
      builder_json["uuid"] = builder.uuid;
      builder_json["name"] = builder.name;
      builder_json["client_config"] = builder.client_config;
      builder_json["status"] = Message::builder_status_to_str(builder.status);
      json.push_back(builder_json);
    }
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
    if (obj["type"] == "get_builders") {
      MessageGetBuilders mes;
      fill_common(mes, obj);
      return AllMessagesVariant{mes};
    }
    if (obj["type"] == "get_builders_response") {
      MessageGetBuildersResponse mes;
      fill_common(mes, obj);
      mes.content.request_uuid = obj["content"]["request_uuid"];
      for (auto builder : obj["content"]["builders"]) {
        mes.content.builders.emplace_back(
            builder["uuid"], builder["name"], builder["client_config"],
            Message::str_to_builder_status(builder["status"].dump()));
      }
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
  // Iterate over all MessageType enum values and run `dispatch_for_type` for it
  template for (constexpr auto mes_type :
                get_all_enum_values<Message::MessageType>()) {
    dispatch_for_type<mes_type>(message);
  }
}
