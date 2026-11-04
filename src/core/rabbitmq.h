#pragma once
#include <map>

#include "message.h"

class RabbitMQ {
  class Internal;

public:
  RabbitMQ(std::string_view address);
  ~RabbitMQ();

  // Has to be run frequently.
  // Checks if the file descriptors needed by AMQP-CPP are ready to be read from
  // or written to. Blocking.
  // `timeout` in ms
  void monitor(int timeout);

protected:
  Internal *internal = nullptr;

private:
  std::map<int, int> monitored_fds;
};

class BuilderRabbitMQ : public RabbitMQ {
public:
  BuilderRabbitMQ(std::string_view address, std::string_view uuid);

  void send_message(const Message &message);

private:
  std::string uuid;
  std::string routing_key;
};
