#include <future>
#include <stop_token>
#include <thread>

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

  std::jthread heartbeat_thread{[&rabbitmq](std::stop_token stoken) {
    auto ready_future = rabbitmq.get_ready_future();
    ready_future.wait();

    while (!stoken.stop_requested()) {
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(5s);

      rabbitmq.heartbeat();
    }
  }};

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
    rabbitmq.monitor(10000);
  }

  heartbeat_thread.request_stop();
}
