#pragma once
#include <chrono>
#include <meta>
#include <optional>
#include <ranges>
#include <string>
#include <variant>
#include <vector>

// Messages sent between the server and the clients
class Message {
public:
  enum MessageType {
    INITIALIZE_BUILDER,

    TASK_REQUEST,
    TASK_REQUEST_RESPONSE,
    TASK_COMPLETED,

    LOG,

    PING,
    PONG,

    DISCOVERY,
    DISCOVERY_RESPONSE,
  };
  static std::string message_type_to_str(MessageType mes_type);

  enum BuilderStatus {
    IDLE,
    WORKING,
  };
  static std::string builder_status_to_str(BuilderStatus builder_status);
  static BuilderStatus str_to_builder_status(std::string_view str);

protected:
  // This class should never be constructed directly
  Message() {}

public:
  std::string message_uuid = "";
  MessageType type;
  std::string sender = "";
};

struct MessageSpecialization {};

class[[= MessageSpecialization{}]] MessageInitializeBuilder : public Message {
public:
  MessageInitializeBuilder() { type = INITIALIZE_BUILDER; }

  struct {
    std::string name = "";
    std::string client_config = "";
  } content;
};

class[[= MessageSpecialization{}]] MessageTaskRequest : public Message {
public:
  MessageTaskRequest() { type = TASK_REQUEST; }

  struct {
    std::string flake_url = "";
    std::string client_config = "";
    std::string task_input = "";
  } content;
};

class[[= MessageSpecialization{}]] MessageTaskRequestResponse : public Message {
public:
  MessageTaskRequestResponse() { type = TASK_REQUEST_RESPONSE; }

  struct {
    std::string request_uuid = "";
    std::string response = "";
  } content;
};

class[[= MessageSpecialization{}]] MessageTaskCompleted : public Message {
public:
  MessageTaskCompleted() { type = TASK_COMPLETED; }

  struct {
    std::string request_uuid = "";
    std::string response = "";
    std::string result = "";
  } content;
};

class[[= MessageSpecialization{}]] MessageLog : public Message {
public:
  MessageLog() { type = LOG; }

  struct {
    std::string message = "";
  } content;
};

class[[= MessageSpecialization{}]] MessagePing : public Message {
public:
  MessagePing() { type = PING; }
};

class[[= MessageSpecialization{}]] MessagePong : public Message {
public:
  MessagePong() { type = PONG; }

  struct {
    std::string ping_uuid = "";
    std::chrono::high_resolution_clock::time_point timestamp;
  } content;
};

class[[= MessageSpecialization{}]] MessageDiscovery : public Message {
public:
  MessageDiscovery() { type = DISCOVERY; }
};

class[[= MessageSpecialization{}]] MessageDiscoveryResponse : public Message {
public:
  MessageDiscoveryResponse() { type = DISCOVERY_RESPONSE; }

  struct {
    std::string request_uuid = "";
    std::string name = "";
    std::string client_config = "";
    BuilderStatus status;
  } content;
};

consteval auto message_specializations() {
  auto ns = std::meta::current_namespace();
  auto members =
      std::meta::members_of(ns, std::meta::access_context::current());

  std::vector<std::meta::info> filtered;
  for (auto member : members) {
    auto annotations = std::meta::annotations_of_with_type(
        member, ^^MessageSpecialization);

    if (!annotations.empty()) filtered.push_back(member);
  }

  return std::define_static_array(filtered);
}

using AllMessagesVariant =
    [:std::meta::substitute(^^std::variant, message_specializations()):];

std::string message_to_json(const Message &mes);
std::optional<AllMessagesVariant> json_to_message(std::string_view json);
