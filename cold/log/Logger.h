#ifndef COLD_LOG_LOGGER
#define COLD_LOG_LOGGER

#include <atomic>
#include <initializer_list>
#include <memory>
#include <source_location>
#include <string>
#include <vector>

#include "cold/log/LogCommon.h"
#include "cold/thread/Lock.h"

namespace Cold::Base {

class LogSink;

class Logger {
 public:
  using SinkPtr = std::shared_ptr<LogSink>;

  Logger(std::string LoggerName) : Logger(LoggerName, {}) {}

  Logger(std::string LoggerName, SinkPtr sink) : Logger(LoggerName, {sink}) {}

  Logger(std::string LoggerName, std::initializer_list<SinkPtr> list)
      : Logger(LoggerName, list.begin(), list.end()) {}

  template <typename It>
  Logger(std::string LoggerName, It begin, It end)
      : name_(std::move(LoggerName)),
        loggerLevel_(LogLevel::INFO),
        flushLevel_(LogLevel::OFF),
        loggerSinks_(begin, end) {}

  ~Logger() = default;

  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  const std::string& GetName() const { return name_; }

  LogLevel GetLevel() const { return loggerLevel_; }
  void SetLevel(LogLevel level) { loggerLevel_ = level; }

  LogLevel GetFlushLevel() const { return flushLevel_; }
  void SetFlushLevel(LogLevel level) { flushLevel_ = level; }

  void DoLog(const LogMessage& message) { SinkIt(message); }

 private:
  void SinkIt(const LogMessage& message);
  bool ShouldFlush(const LogMessage& message) const;
  void Flush();

  std::string name_;
  std::atomic<LogLevel> loggerLevel_;
  std::atomic<LogLevel> flushLevel_;
  std::vector<SinkPtr> loggerSinks_;
};

struct LocationWrapper {
  constexpr LocationWrapper(
      std::source_location loc = std::source_location::current()) noexcept
      : location(loc) {
    baseName = location.file_name();
    auto pos = baseName.find_last_of("/");
    if (pos != std::string_view::npos) {
      baseName = baseName.substr(pos + 1);
    }
  }
  std::source_location location;
  std::string_view baseName;
};

}  // namespace Cold::Base

#endif /* COLD_LOG_LOGGER */
