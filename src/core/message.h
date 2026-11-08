#pragma once
#include <chrono>
#include <string>

// Messages sent between the server and the clients
class Message {
public:
  enum MessageType {
    INITIALIZE_BUILDER,
    INITIALIZE_BUILDER_RESPONSE,

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

class MessageInitializeBuilderResponse : public Message {
public:
  MessageInitializeBuilderResponse() { type = INITIALIZE_BUILDER_RESPONSE; }

  struct {
    std::string response_uuid = "";
    std::string response = "";
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

std::string message_to_json(const Message &mes);
