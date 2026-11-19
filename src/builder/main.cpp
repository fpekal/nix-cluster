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

  MessageDispatcher message_dispatcher;

  message_dispatcher.bind_handler(
      Message::PING,
      std::make_shared<MessageDispatcher::Handler>(
          [client_uuid, &rabbitmq](void *, const AllMessagesVariant &message) {
            Logger::get_logger().log(DEBUG, "Got PING");
            auto &m = std::get<MessagePing>(message);
            MessagePong new_message;
            new_message.sender = client_uuid;
            new_message.message_uuid = UUID::generate();
            new_message.content.ping_uuid = m.message_uuid;
            new_message.content.timestamp =
                std::chrono::high_resolution_clock::now();

            rabbitmq.send_message(new_message);
            Logger::get_logger().log(DEBUG, "Sent PONG");
          }));

  message_dispatcher.bind_handler(
      Message::DISCOVERY,
      std::make_shared<MessageDispatcher::Handler>(
          [client_uuid, &rabbitmq](void *, const AllMessagesVariant &message) {
            Logger::get_logger().log(DEBUG, "Got DISCOVERY");
            auto &m = std::get<MessageDiscovery>(message);
            MessageDiscoveryResponse new_message;
            new_message.sender = client_uuid;
            new_message.message_uuid = UUID::generate();
            new_message.content.request_uuid = m.message_uuid;
            new_message.content.name = "Joshua";
            new_message.content.client_config = "{amo=\"gus\";}";
            new_message.content.status = BuilderStatus::IDLE;

            rabbitmq.send_message(new_message);
            Logger::get_logger().log(DEBUG, "Sent DISCOVERY_RESPONSE");
          }));

  rabbitmq.message_received_handler =
      [&message_dispatcher](const AllMessagesVariant &message) {
        message_dispatcher.dispatch(message);
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

    Logger::get_logger().log(INFO, "Waiting for tasks");
  });

  while (true) {
    rabbitmq.monitor(10000);
  }

  heartbeat_thread.request_stop();
}
