#include <future>
#include <stop_token>
#include <thread>
#include <variant>

#include <logger.h>
#include <message.h>
#include <rabbitmq.h>
#include <uuid.h>

#include "builder.h"
#include "cluster.h"

int main() {
  // Builders builders;

  Logger::get_logger().set_level(LoggerLevel::INFO);
  Cluster cluster{"amqp://nix-cluster:develop@amogus:5672/nix-cluster"};

  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds{5});
  }

  // ServerRabbitMQ
  // rabbitmq{"amqp://nix-cluster:develop@amogus:5672/nix-cluster"};

  // std::jthread heartbeat_thread{[&rabbitmq](std::stop_token stoken) {
  //   auto ready_future = rabbitmq.get_ready_future();
  //   ready_future.wait();

  //   while (!stoken.stop_requested()) {
  //     using namespace std::chrono_literals;
  //     std::this_thread::sleep_for(5s);

  //     rabbitmq.heartbeat();
  //   }
  // }};

  // rabbitmq.message_received_handler =
  //     [&builders](const AllMessagesVariant &message_variant) {
  //       if
  //       (std::holds_alternative<MessageInitializeBuilder>(message_variant)) {
  //         // Save builder's info
  //         auto message = std::get<MessageInitializeBuilder>(message_variant);
  //         builders[message.sender] =
  //             BuilderInfo{message.sender, message.content.name,
  //                         message.content.client_config};
  //       } else if (std::holds_alternative<MessageLog>(message_variant)) {
  //         // Passthrough logs from builders to the servers
  //         auto message = std::get<MessageLog>(message_variant);
  //         auto &builder_info = builders[message.sender];
  //         Logger::get_logger().log(LoggerLevel::INFO, "[{}] {}",
  //                                  builder_info.name,
  //                                  message.content.message);
  //       }
  //     };

  // auto foo = std::async([&rabbitmq]() {
  //   auto ready_future = rabbitmq.get_ready_future();
  //   ready_future.wait();

  //   MessageLog log;
  //   log.content.message = "Amognus";
  //   log.message_uuid = UUID::generate();
  //   log.sender = "server";

  //   rabbitmq.send_message_broadcast(log);

  //   Logger::get_logger().log(INFO, "Log message sent");
  // });

  // while (true) {
  //   rabbitmq.monitor(10000);
  // }

  // heartbeat_thread.request_stop();
}
