#include <chrono>

#include <logger.h>
#include <message.h>

int main() {
  Logger logger;

  MessagePong mes;
  mes.message_uuid = "2137";
  mes.content.ping_uuid = "Amogus";
  mes.content.timestamp = std::chrono::high_resolution_clock::now();

  logger.log(INFO, "{}", message_to_json(mes));
}
