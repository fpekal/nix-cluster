#pragma once
#include <format>
#include <memory>
#include <mutex>
#include <string>

class LoggerSink {
public:
  virtual ~LoggerSink() {}

  virtual void send_raw_message(std::string_view) = 0;
  virtual void flush() = 0;
};

class LoggerSinkStdout : public LoggerSink {
public:
  virtual void send_raw_message(std::string_view) override;
  virtual void flush() override;
};

enum LoggerLevel { NONE = 0, ERROR = 1, WARN = 2, INFO = 3, DEBUG = 4 };

class Logger {
public:
  static Logger &get_logger() { return global_logger; }

private:
  static Logger global_logger;

public:
  // Logger ignores messages with LARGER number than currently set one
  static std::string level_to_str(LoggerLevel level);

  Logger();

  template <class... Args>
  void log(LoggerLevel level, std::format_string<Args...> fmt, Args &&...args) {
    if (level > this->level)
      return;
    std::string formatted = std::format(fmt, std::forward<Args>(args)...);
    std::string level_str = level_to_str(level);

    std::string added_level = std::format("[{}] {}", level_str, formatted);
    std::lock_guard guard{mutex};
    sink->send_raw_message(added_level);
  }

  void set_sink(std::unique_ptr<LoggerSink> &&sink);
  void set_level(LoggerLevel level);
  void flush();

private:
  std::unique_ptr<LoggerSink> sink;
  LoggerLevel level;
  std::mutex mutex;
};
