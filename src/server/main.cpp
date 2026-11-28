#include <future>
#include <stop_token>
#include <thread>
#include <variant>

#include <builder.h>
#include <logger.h>
#include <message.h>
#include <rabbitmq.h>
#include <uuid.h>

#include "cluster.h"

int main(int argc, char *argv[]) {
  Logger::get_logger().set_level(LoggerLevel::DEBUG);
  if (argc != 2) {
    Logger::get_logger().log(ERROR, "Expected 1 argument: amqp_uri");
  }

  // amqp://nix-cluster:develop@amogus:5672/nix-cluster
  Cluster cluster{argv[1]};

  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds{5});
  }
}
