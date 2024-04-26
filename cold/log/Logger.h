#ifndef COLD_LOG_LOGGER
#define COLD_LOG_LOGGER

#include <atomic>
#include <memory>

#include "cold/log/LogCommon.h"
#include "cold/log/LogFormatter.h"
#include "cold/thread/Lock.h"
#include "cold/thread/Thread.h"
#include "cold/time/Time.h"

namespace Cold::Base {

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

class LogSink;
class LogFormatter;

class Logger {
 public:
  using SinkPtr = std::shared_ptr<LogSink>;
  using LogFormatterPtr = std::unique_ptr<LogFormatter>;

  Logger(std::string LoggerName) : Logger(LoggerName, {}) {}

  Logger(std::string LoggerName, SinkPtr sink) : Logger(LoggerName, {sink}) {}

  Logger(std::string LoggerName, std::initializer_list<SinkPtr> &&list)
      : Logger(LoggerName, list.begin(), list.end()) {}

  template <typename It>
  Logger(std::string LoggerName, It begin, It end)
      : name_(std::move(LoggerName)),
        loggerLevel_(LogLevel::INFO),
        flushLevel_(LogLevel::OFF),
        loggerSinks_(begin, end) {}
  ~Logger() = default;

  Logger(const Logger &) = delete;
  Logger &operator=(const Logger &) = delete;

  const std::string &GetName() const { return name_; }
  LogLevel GetLevel() const { return loggerLevel_; }
  void SetLevel(LogLevel level) { loggerLevel_ = level; }

  LogLevel GetFlushLevel() const { return flushLevel_; }
  void SetFlushLevel(LogLevel level) { flushLevel_ = level; }

  void SetPattern(std::string_view pattern);
  void SetFormatter(LogFormatterPtr formatter);

  const std::vector<SinkPtr> &Sinks() const { return loggerSinks_; }

  template <typename... Args>
  void Log(LogLevel level, LocationWrapper &wrapper,
           fmt::format_string<Args...> fmt, Args &&...args) {
    LogMessage message;
    message.level = level;
    message.threadId = ThisThread::ThreadIdStr();
    message.threadName = ThisThread::ThreadName();
    message.loggerName = name_;
    message.location = wrapper.location;
    message.baseName = wrapper.baseName;
    message.logTime = Time::Now();
    auto logLine = fmt::format(fmt, std::forward<Args>(args)...);
    message.logLine = logLine;
    SinkIt(message);
  }

 private:
  void SinkIt(const LogMessage &message);
  bool ShouldFlush(const LogMessage &message) const;
  void Flush();

  std::string name_;
  std::atomic<LogLevel> loggerLevel_;
  std::atomic<LogLevel> flushLevel_;
  std::vector<SinkPtr> loggerSinks_;
};

class LogManager {
 public:
  using LoggerPtr = std::shared_ptr<Logger>;
  ~LogManager() = default;
  LogManager(const LogManager &) = delete;
  LogManager &operator=(const LogManager &) = delete;

  static LogManager &Instance();

  LoggerPtr GetMainLogger();
  void SetMainLogger(LoggerPtr logger);
  // thread unsafe
  Logger *GetMainLoggerRaw();

  LoggerPtr GetLogger(const std::string &loggerName);
  bool RemoveLogger(const std::string &loggerName);
  void AddLogger(LoggerPtr logger);

 private:
  LogManager();
  Mutex mutex_;
  LoggerPtr mainLogger_;
  std::unordered_map<std::string, LoggerPtr> loggersMap_ GUARDED_BY(mutex_);
};

using LoggerPtr = LogManager::LoggerPtr;

// This method take from https://zhuanlan.zhihu.com/p/620056922

// for specific logger
template <typename... Args>
struct Trace {
  constexpr Trace(const LoggerPtr &logger, fmt::format_string<Args...> fmt,
                  Args &&...args, LocationWrapper wrap = {}) {
    logger->Log(LogLevel::TRACE, wrap, fmt, std::forward<Args>(args)...);
  }
};
template <typename... Args>
Trace(fmt::format_string<Args...> fmt, Args &&...args) -> Trace<Args...>;

template <typename... Args>
struct Debug {
  constexpr Debug(const LoggerPtr &logger, fmt::format_string<Args...> fmt,
                  Args &&...args, LocationWrapper wrap = {}) {
    logger->Log(LogLevel::DEBUG, wrap, fmt, std::forward<Args>(args)...);
  }
};
template <typename... Args>
Debug(fmt::format_string<Args...> fmt, Args &&...args) -> Debug<Args...>;

template <typename... Args>
struct Info {
  constexpr Info(const LoggerPtr &logger, fmt::format_string<Args...> fmt,
                 Args &&...args, LocationWrapper wrap = {}) {
    logger->Log(LogLevel::INFO, wrap, fmt, std::forward<Args>(args)...);
  }
};
template <typename... Args>
Info(fmt::format_string<Args...> fmt, Args &&...args) -> Info<Args...>;

template <typename... Args>
struct Warn {
  constexpr Warn(const LoggerPtr &logger, fmt::format_string<Args...> fmt,
                 Args &&...args, LocationWrapper wrap = {}) {
    logger->Log(LogLevel::WARN, wrap, fmt, std::forward<Args>(args)...);
  }
};
template <typename... Args>
Warn(fmt::format_string<Args...> fmt, Args &&...args) -> Warn<Args...>;

template <typename... Args>
struct Error {
  constexpr Error(const LoggerPtr &logger, fmt::format_string<Args...> fmt,
                  Args &&...args, LocationWrapper wrap = {}) {
    logger->Log(LogLevel::ERROR, wrap, fmt, std::forward<Args>(args)...);
  }
};
template <typename... Args>
Error(fmt::format_string<Args...> fmt, Args &&...args) -> Error<Args...>;

template <typename... Args>
struct Fatal {
  constexpr Fatal(const LoggerPtr &logger, fmt::format_string<Args...> fmt,
                  Args &&...args, LocationWrapper wrap = {}) {
    logger->Log(LogLevel::FATAL, wrap, fmt, std::forward<Args>(args)...);
  }
};
template <typename... Args>
Fatal(fmt::format_string<Args...> fmt, Args &&...args) -> Fatal<Args...>;

/// for default logger use raw ptr do log
template <typename... Args>
struct TRACE {
  constexpr TRACE(fmt::format_string<Args...> fmt, Args &&...args,
                  LocationWrapper wrap = {}) {
    LogManager::Instance().GetMainLoggerRaw()->Log(LogLevel::TRACE, wrap, fmt,
                                                   std::forward<Args>(args)...);
  }
};
template <typename... Args>
TRACE(fmt::format_string<Args...> fmt, Args &&...args) -> TRACE<Args...>;

template <typename... Args>
struct DEBUG {
  constexpr DEBUG(fmt::format_string<Args...> fmt, Args &&...args,
                  LocationWrapper wrap = {}) {
    LogManager::Instance().GetMainLoggerRaw()->Log(LogLevel::DEBUG, wrap, fmt,
                                                   std::forward<Args>(args)...);
  }
};
template <typename... Args>
DEBUG(fmt::format_string<Args...> fmt, Args &&...args) -> DEBUG<Args...>;

template <typename... Args>
struct INFO {
  constexpr INFO(fmt::format_string<Args...> fmt, Args &&...args,
                 LocationWrapper wrap = {}) {
    LogManager::Instance().GetMainLoggerRaw()->Log(LogLevel::INFO, wrap, fmt,
                                                   std::forward<Args>(args)...);
  }
};
template <typename... Args>
INFO(fmt::format_string<Args...> fmt, Args &&...args) -> INFO<Args...>;

template <typename... Args>
struct WARN {
  constexpr WARN(fmt::format_string<Args...> fmt, Args &&...args,
                 LocationWrapper wrap = {}) {
    LogManager::Instance().GetMainLoggerRaw()->Log(LogLevel::WARN, wrap, fmt,
                                                   std::forward<Args>(args)...);
  }
};
template <typename... Args>
WARN(fmt::format_string<Args...> fmt, Args &&...args) -> WARN<Args...>;

template <typename... Args>
struct ERROR {
  constexpr ERROR(fmt::format_string<Args...> fmt, Args &&...args,
                  LocationWrapper wrap = {}) {
    LogManager::Instance().GetMainLoggerRaw()->Log(LogLevel::ERROR, wrap, fmt,
                                                   std::forward<Args>(args)...);
  }
};
template <typename... Args>
ERROR(fmt::format_string<Args...> fmt, Args &&...args) -> ERROR<Args...>;

template <typename... Args>
struct FATAL {
  constexpr FATAL(fmt::format_string<Args...> fmt, Args &&...args,
                  LocationWrapper wrap = {}) {
    LogManager::Instance().GetMainLoggerRaw()->Log(LogLevel::FATAL, wrap, fmt,
                                                   std::forward<Args>(args)...);
  }
};
template <typename... Args>
FATAL(fmt::format_string<Args...> fmt, Args &&...args) -> FATAL<Args...>;

}  // namespace Cold::Base

#endif /* COLD_LOG_LOGGER */
