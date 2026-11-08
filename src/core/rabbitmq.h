#pragma once
#include <future>
#include <map>
#include <mutex>

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
  // DON'T CALL THIS FUNCTION WITH LOCKED MUTEX!
  // This function locks it when it's neccesary
  void monitor(int timeout);

  // Used to know when it's safe to send messages etc
  std::shared_future<void> get_ready_future() {
    std::lock_guard guard{mutex};
    return ready_future;
  }

  // Lock this mutex before using any functionality of RabbitMQ.
  std::mutex mutex;

protected:
  Internal *internal = nullptr;

  // Fires when all setup has been completed
  std::promise<void> ready_promise;

private:
  std::map<int, int> monitored_fds;

  // Fires when all setup has been completed
  std::shared_future<void> ready_future;
};

class BuilderRabbitMQ : public RabbitMQ {
public:
  BuilderRabbitMQ(std::string_view address, std::string_view uuid);

  void send_message(const Message &message);

private:
  std::string uuid;
  std::string routing_key;
};
