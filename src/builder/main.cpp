#include <iostream>

#include <message.h>
#include <rabbitmq.h>
#include <uuid.h>

int main() {
  MessageLog log;
  log.content.message = "Amognus";

  for (int i = 0; i < 16; ++i) {
    auto uuid = UUID::generate();
    std::cout << std::string{uuid} << '\n';
  }

  BuilderRabbitMQ rabbitmq{"amqp://guest:guest@localhost:5672", "moje_uuid"};

  rabbitmq.send_message(log);

  using namespace std::chrono_literals;
  while (true) {
    rabbitmq.monitor(1000);
    std::cout << "." << std::flush;
  }
}
