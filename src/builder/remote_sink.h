#pragma once
#include <queue>

#include <logger.h>
#include <rabbitmq.h>

class LoggerSinkRemote : public LoggerSink {
public:
  LoggerSinkRemote(std::shared_ptr<BuilderRabbitMQ> &rabbitmq,
                   std::string_view client_uuid)
      : rabbitmq{rabbitmq}, client_uuid{client_uuid} {}

  virtual void send_raw_message(std::string_view) override;
  virtual void flush() override;

private:
  std::shared_ptr<BuilderRabbitMQ> rabbitmq;
  std::string client_uuid;

  // When not connected to the rabbitmq, add log to the queue send them on the
  // next occasion.
  std::queue<std::string> queue;
};
