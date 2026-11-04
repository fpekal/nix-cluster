#include <iostream>

#include <message.h>
#include <rabbitmq.h>

int main() {
  MessageLog log;
  log.content.message = "Amognus";

  BuilderRabbitMQ rabbitmq{"amqp://guest:guest@localhost:5672", "moje_uuid"};

  rabbitmq.send_message(log);

  using namespace std::chrono_literals;
  while (true) {
    rabbitmq.monitor(1000);
    std::cout << "." << std::flush;
  }
}
