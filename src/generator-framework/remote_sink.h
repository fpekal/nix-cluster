#pragma once
#include <queue>

#include <logger.h>

#include "framework.h"

class LoggerSinkRemote : public LoggerSink {
public:
  LoggerSinkRemote(Framework &framework) : framework{framework} {}

  virtual void send_raw_message(std::string_view) override;
  virtual void flush() override;

private:
  Framework &framework;
};
