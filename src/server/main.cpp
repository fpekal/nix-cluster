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
  Logger::get_logger().set_level(LoggerLevel::DEBUG);
  Cluster cluster{"amqp://nix-cluster:develop@amogus:5672/nix-cluster"};

  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds{5});
  }
}
