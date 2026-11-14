#include <future>
#include <stop_token>
#include <thread>

#include <logger.h>
#include <message.h>
#include <rabbitmq.h>
#include <uuid.h>

int main() {
  Logger::get_logger().set_level(LoggerLevel::DEBUG);

  ServerRabbitMQ rabbitmq{"amqp://nix-cluster:develop@amogus:5672/nix-cluster"};

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
    log.sender = "server";

    rabbitmq.send_message_broadcast(log);

    Logger::get_logger().log(INFO, "Log message sent");
  });

  while (true) {
    rabbitmq.monitor(10000);
  }

  heartbeat_thread.request_stop();
}
