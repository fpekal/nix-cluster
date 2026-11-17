#include <chrono>
#include <future>
#include <stop_token>
#include <thread>
#include <variant>

#include <logger.h>
#include <message.h>
#include <rabbitmq.h>
#include <uuid.h>

#include "remote_sink.h"

int main() {
  Logger::get_logger().set_level(LoggerLevel::INFO);

  UUID client_uuid = UUID::generate();
  Logger::get_logger().log(LoggerLevel::INFO, "Client UUID: {}",
                           std::string{client_uuid});
  BuilderRabbitMQ rabbitmq{"amqp://nix-cluster:develop@amogus:5672/nix-cluster",
                           std::string{client_uuid}};

  auto remote_sink =
      std::make_unique<LoggerSinkRemote>(rabbitmq, std::string{client_uuid});
  Logger::get_logger().set_sink(std::move(remote_sink));

  rabbitmq.message_received_handler =
      [client_uuid, &rabbitmq](const AllMessagesVariant &message) {
        if (std::holds_alternative<MessagePing>(message)) {
          Logger::get_logger().log(DEBUG, "Got PING");
          auto &m = std::get<MessagePing>(message);
          MessagePong message;
          message.sender = client_uuid;
          message.message_uuid = UUID::generate();
          message.content.ping_uuid = m.message_uuid;
          message.content.timestamp = std::chrono::high_resolution_clock::now();

          rabbitmq.send_message(message);
          Logger::get_logger().log(DEBUG, "Sent PONG");
        }
      };

  std::jthread heartbeat_thread{[&rabbitmq](std::stop_token stoken) {
    auto ready_future = rabbitmq.get_ready_future();
    ready_future.wait();

    while (!stoken.stop_requested()) {
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(5s);

      rabbitmq.heartbeat();
    }
  }};

  auto foo = std::async([&rabbitmq, client_uuid]() {
    auto ready_future = rabbitmq.get_ready_future();
    ready_future.wait();

    {
      MessageInitializeBuilder init_build;
      init_build.message_uuid = UUID::generate();
      init_build.sender = client_uuid;
      init_build.content.name = "Joshua";
      init_build.content.client_config = "{amo=\"gus\";}";

      rabbitmq.send_message(init_build);
    }

    {
      MessageLog log;
      log.content.message = "Amognus";
      log.message_uuid = UUID::generate();
      log.sender = client_uuid;

      rabbitmq.send_message(log);
    }

    Logger::get_logger().log(INFO, "Log message sent");
  });

  while (true) {
    rabbitmq.monitor(10000);
  }

  heartbeat_thread.request_stop();
}
