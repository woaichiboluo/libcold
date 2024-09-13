#ifndef COLD_LOG_LOGGER
#define COLD_LOG_LOGGER

#include <atomic>
#include <vector>

#include "../thread/Thread.h"
#include "logsinks/LogSink.h"

namespace Cold {

namespace detail {

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

}  // namespace detail

class Logger {
 public:
  using SinkPtr = std::shared_ptr<LogSink>;
  using LogFormatterPtr = std::unique_ptr<LogFormatter>;

  explicit Logger(std::string LoggerName) : Logger(LoggerName, {}) {}
  Logger(std::string LoggerName, SinkPtr sink) : Logger(LoggerName, {sink}) {}
  Logger(std::string LoggerName, std::initializer_list<SinkPtr> &&list)
      : Logger(LoggerName, list.begin(), list.end()) {}

  template <typename It>
  Logger(std::string LoggerName, It begin, It end)
      : name_(std::move(LoggerName)),
        sinks_(begin, end),
        loggerLevel_(LogLevel::INFO),
        flushLevel_(LogLevel::OFF) {}

  ~Logger() = default;

  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;

  const std::string &GetName() const { return name_; }
  LogLevel GetLevel() const { return loggerLevel_; }
  void SetLevel(LogLevel level) { loggerLevel_ = level; }

  LogLevel GetFlushLevel() const { return flushLevel_; }
  void SetFlushLevel(LogLevel level) { flushLevel_ = level; }

  void SetPattern(std::string_view pattern) {
    for (auto &sink : sinks_) {
      sink->SetPattern(pattern);
    }
  }

  void SetFormatter(LogFormatterPtr formatter) {
    for (auto &sink : sinks_) {
      sink->SetFormatter(formatter->Clone());
    }
  }

  template <typename... Args>
  void Log(LogLevel level, detail::LocationWrapper w,
           fmt::format_string<Args...> fmt, Args &&...args) {
    LogMessage message;
    message.logTime = Time::Now();
    message.level = level;
    message.baseName = w.baseName;
    message.location = w.location;
    message.loggerName = name_;
    auto logLine = fmt::format(fmt, std::forward<Args>(args)...);
    message.logLine = logLine;
    // FIXME
    message.threadName = ThisThread::ThreadName();
    auto id = ThisThread::ThreadIdStr();
    message.threadId = id;
    SinkIt(message);
  }

 private:
  void SinkIt(const LogMessage &message) {
    if (loggerLevel_ == LogLevel::OFF) return;
    assert(message.level < LogLevel::OFF);
    assert(loggerLevel_ <= LogLevel::FATAL);
    if (message.level >= loggerLevel_) {
      for (auto &sink : sinks_) sink->Sink(message);
    }
    if (ShouldFlush(message)) Flush();
    if (message.level == LogLevel::FATAL) abort();
  }

  bool ShouldFlush(const LogMessage &message) const {
    return message.level != LogLevel::OFF &&
           message.level >= flushLevel_.load();
  }

  void Flush() {
    for (auto &sink : sinks_) {
      sink->Flush();
    }
  }

  std::string name_;
  std::vector<SinkPtr> sinks_;
  std::atomic<LogLevel> loggerLevel_;
  std::atomic<LogLevel> flushLevel_;
};

}  // namespace Cold

#endif /* COLD_LOG_LOGGER */
