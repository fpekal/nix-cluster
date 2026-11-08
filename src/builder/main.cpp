#include <future>

#include <logger.h>
#include <message.h>
#include <rabbitmq.h>
#include <uuid.h>

int main() {
  Logger::get_logger().set_level(LoggerLevel::DEBUG);

  UUID client_uuid = UUID::generate();
  Logger::get_logger().log(LoggerLevel::INFO, "Client UUID: {}",
                           std::string{client_uuid});
  BuilderRabbitMQ rabbitmq{"amqp://guest:guest@localhost:5672",
                           std::string{client_uuid}};

  auto foo = std::async([&rabbitmq]() {
    auto ready_future = rabbitmq.get_ready_future();

    ready_future.wait();

    MessageLog log;
    log.content.message = "Amognus";
    log.message_uuid = UUID::generate();

    rabbitmq.send_message(log);

    Logger::get_logger().log(INFO, "Log message sent");
  });

  while (true) {
    rabbitmq.monitor(5000);
  }
}
