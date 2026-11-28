#include "logger.h"

#include <iostream>
#include <ostream>

#include "util.h"

void LoggerSinkStdout::send_raw_message(std::string_view sv) {
  std::cout << sv << '\n';
}

void LoggerSinkStdout::flush() { std::cout << std::flush; }

std::string Logger::level_to_str(LoggerLevel level) {
  static constexpr auto levels_mappings =
      generate_enum_to_string_view_array<LoggerLevel>();

  for (const auto &mapping : levels_mappings) {
    if (level == mapping.first)
      return std::string{mapping.second};
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

void Logger::flush() { sink->flush(); }

Logger Logger::global_logger{};
