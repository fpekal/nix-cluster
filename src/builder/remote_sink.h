#pragma once
#include <logger.h>
#include <rabbitmq.h>

class LoggerSinkRemote : public LoggerSink {
public:
  LoggerSinkRemote(BuilderRabbitMQ &rabbitmq, std::string_view client_uuid)
      : rabbitmq{rabbitmq}, client_uuid{client_uuid} {}

  virtual void send_raw_message(std::string_view) override;
  virtual void flush() override;

private:
  BuilderRabbitMQ &rabbitmq;
  std::string client_uuid;
};
