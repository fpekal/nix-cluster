#include <future>
#include <iostream>

#include <message.h>
#include <rabbitmq.h>
#include <uuid.h>

int main() {
  BuilderRabbitMQ rabbitmq{"amqp://guest:guest@localhost:5672",
                           std::string{UUID::generate()}};

  auto foo = std::async([&rabbitmq]() {
    rabbitmq.mutex.lock();
    auto ready_future = rabbitmq.get_ready_future();
    rabbitmq.mutex.unlock();

    ready_future.wait();

    MessageLog log;
    log.content.message = "Amognus";
    log.message_uuid = UUID::generate();

    rabbitmq.mutex.lock();
    rabbitmq.send_message(log);
    rabbitmq.mutex.unlock();
  });

  while (true) {
    rabbitmq.monitor(1000);
    std::cout << "." << std::flush;
  }
}
