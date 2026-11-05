#include "logger.h"

#include <iostream>
#include <ostream>

void LoggerSinkStdout::send_raw_message(std::string_view sv) {
  std::cout << sv << '\n';
}

void LoggerSinkStdout::flush() { std::cout << std::flush; }

std::string Logger::level_to_str(LoggerLevel level) {
  switch (level) {
  case NONE:
    return "NONE";
  case ERROR:
    return "ERROR";
  case WARN:
    return "WARN";
  case INFO:
    return "INFO";
  case DEBUG:
    return "DEBUG";
  }

  return "UNKNOWN";
}

Logger::Logger() {
  sink = std::make_unique<LoggerSinkStdout>();
  level = INFO;
}

void Logger::set_sink(std::unique_ptr<LoggerSink> &&sink) {
  this->sink = std::move(sink);
}

void Logger::set_level(LoggerLevel level) { this->level = level; }

void Logger::flush() { sink->flush(); }
