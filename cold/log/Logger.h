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
#include "cold/thread/Thread.h"

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

using LoggerPtr = std::shared_ptr<Logger>;

LoggerPtr GetMainLogger();
void SetMainLogger(LoggerPtr logger);

LoggerPtr GetLogger(const std::string& name);
bool RemoveLogger(const std::string& name);
void RegisterLogger(LoggerPtr logger);

#define DOLOG(logger, logLevel, formatStr, args...)            \
  do {                                                         \
    constexpr Cold::Base::LocationWrapper wrapper;             \
    Cold::Base::LogMessage message;                            \
    message.level = logLevel;                                  \
    message.threadId = Cold::Base::ThisThread::ThreadIdStr();  \
    message.threadName = Cold::Base::ThisThread::ThreadName(); \
    message.loggerName = logger->GetName();                    \
    message.location = wrapper.location;                       \
    message.baseName = wrapper.baseName;                       \
    message.logTime = Cold::Base::Time::Now();                 \
    auto logLine = fmt::format(formatStr, ##args);             \
    message.logLine = logLine;                                 \
    logger->DoLog(message);                                    \
  } while (0)

#define LOG_TRACE(logger, formatStr, args...) \
  DOLOG(logger, Cold::Base::LogLevel::TRACE, formatStr, ##args)

#define LOG_DEBUG(logger, formatStr, args...) \
  DOLOG(logger, Cold::Base::LogLevel::DEBUG, formatStr, ##args)

#define LOG_INFO(logger, formatStr, args...) \
  DOLOG(logger, Cold::Base::LogLevel::INFO, formatStr, ##args)

#define LOG_WARN(logger, formatStr, args...) \
  DOLOG(logger, Cold::Base::LogLevel::WARN, formatStr, ##args)

#define LOG_ERROR(logger, formatStr, args...) \
  DOLOG(logger, Cold::Base::LogLevel::ERROR, formatStr, ##args)

#define LOG_FATAL(logger, formatStr, args...) \
  DOLOG(logger, Cold::Base::LogLevel::FATAL, formatStr, ##args)

}  // namespace Cold::Base

#endif /* COLD_LOG_LOGGER */
