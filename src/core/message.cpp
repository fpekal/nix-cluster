#include "message.h"

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
  if (obj["type"] == "initialize_builder_response") {
    MessageInitializeBuilderResponse mes;
    fill_common(mes, obj);
    mes.content.response = obj["content"]["response"];
    mes.content.response_uuid = obj["content"]["response_uuid"];
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
            static_cast<uint64_t>(obj["content"]["response_uuid"])}};
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
    mes.content.status = obj["content"]["status"];
    return AllMessagesVariant{mes};
  }

  return {};
}
