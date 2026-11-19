#pragma once
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <meta>
#include <mutex>
#include <optional>
#include <ranges>
#include <string>
#include <thread>
#include <variant>
#include <vector>

#include "builder.h"

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

class MessageInitializeBuilder : public Message {
public:
  MessageInitializeBuilder() { type = INITIALIZE_BUILDER; }

  struct {
    std::string name = "";
    std::string client_config = "";
  } content;
};

class MessageTaskRequest : public Message {
public:
  MessageTaskRequest() { type = TASK_REQUEST; }

  struct {
    std::string flake_url = "";
    std::string client_config = "";
    std::string task_input = "";
  } content;
};

class MessageTaskRequestResponse : public Message {
public:
  MessageTaskRequestResponse() { type = TASK_REQUEST_RESPONSE; }

  struct {
    std::string request_uuid = "";
    std::string response = "";
  } content;
};

class MessageTaskCompleted : public Message {
public:
  MessageTaskCompleted() { type = TASK_COMPLETED; }

  struct {
    std::string request_uuid = "";
    std::string response = "";
    std::string result = "";
  } content;
};

class MessageLog : public Message {
public:
  MessageLog() { type = LOG; }

  struct {
    std::string message = "";
  } content;
};

class MessagePing : public Message {
public:
  MessagePing() { type = PING; }
};

class MessagePong : public Message {
public:
  MessagePong() { type = PONG; }

  struct {
    std::string ping_uuid = "";
    std::chrono::high_resolution_clock::time_point timestamp;
  } content;
};

class MessageDiscovery : public Message {
public:
  MessageDiscovery() { type = DISCOVERY; }
};

class MessageDiscoveryResponse : public Message {
public:
  MessageDiscoveryResponse() { type = DISCOVERY_RESPONSE; }

  struct {
    std::string request_uuid = "";
    std::string name = "";
    std::string client_config = "";
    BuilderStatus status;
  } content;
};

// Get all classes in the global scope that inherits `Message` and return
// std::span<std::meta::info> of them
consteval auto message_specializations() {
  auto ns = std::meta::current_namespace();
  auto context = std::meta::access_context::current();
  auto members = std::meta::members_of(ns, context);

  std::vector<std::meta::info> filtered;
  for (auto member : members) {
    if (!std::meta::is_type(member) || !std::meta::is_class_type(member) ||
        !std::meta::is_complete_type(member))
      continue;

    for (auto base : std::meta::bases_of(member, context)) {
      if (std::meta::type_of(base) == ^^Message) {
        filtered.push_back(member);
        break;
      }
    }
  }

  return std::define_static_array(filtered);
}

// std::variant<...> of all classes that inherits `Message`
using AllMessagesVariant = [:std::meta::substitute(^^std::variant,
                                                   message_specializations()):];

std::string message_to_json(const Message &mes);
std::optional<AllMessagesVariant> json_to_message(std::string_view json);

// Allows binding handlers that will run when some message type will arrive
class MessageDispatcher {
public:
  typedef std::function<void(void *, const AllMessagesVariant &)> Handler;

  MessageDispatcher();

  // All handlers are run when some kind of message is received.
  void bind_handler(Message::MessageType type,
                    std::shared_ptr<Handler> handler);

  // `handler` is the value returned by the `bind_handler()` function.
  // Returns true if the handler was found and unbound.
  bool unbind_handler(Message::MessageType type,
                      std::shared_ptr<Handler> handler);

  void dispatch(const AllMessagesVariant &message);

private:
  // When rabbitmq receives a message, it gets handled by ALL handlers of this
  // type of the message.
  std::map<Message::MessageType, std::vector<std::shared_ptr<Handler>>>
      message_handlers;
  std::mutex message_handlers_mutex;
};
