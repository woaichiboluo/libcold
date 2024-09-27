#ifndef COLD_LOG_LOG
#define COLD_LOG_LOG

#include "LogManager.h"
#include "logsinks/StdoutColorSink.h"

namespace Cold {

inline LogManager::LogManager()
    : defaultLogger_(std::make_shared<Logger>(
          "main", std::make_shared<StdoutColorSink>())) {}

using LoggerPtr = std::shared_ptr<Logger>;
#define _COLD_OUTPUT_IMPL(LOGLEVEL, NAME)                                    \
  template <typename... Args>                                                \
  struct NAME {                                                              \
    constexpr NAME(const LoggerPtr &logger, fmt::format_string<Args...> fmt, \
                   Args &&...args, Detail::LocationWrapper wrap = {}) {      \
      logger->Log(LOGLEVEL, wrap, fmt, std::forward<Args>(args)...);         \
    }                                                                        \
    constexpr NAME(Logger *logger, fmt::format_string<Args...> fmt,          \
                   Args &&...args, Detail::LocationWrapper wrap = {}) {      \
      logger->Log(LOGLEVEL, wrap, fmt, std::forward<Args>(args)...);         \
    }                                                                        \
    constexpr NAME(fmt::format_string<Args...> fmt, Args &&...args,          \
                   Detail::LocationWrapper wrap = {}) {                      \
      LogManager::GetInstance().GetDefaultRaw()->Log(                        \
          LOGLEVEL, wrap, fmt, std::forward<Args>(args)...);                 \
    }                                                                        \
  };                                                                         \
  template <typename... Args>                                                \
  NAME(fmt::format_string<Args...> fmt, Args &&...args) -> NAME<Args...>;    \
  template <typename... Args>                                                \
  NAME(Logger *logger, fmt::format_string<Args...> fmt, Args &&...args)      \
      -> NAME<Args...>;                                                      \
  template <typename... Args>                                                \
  NAME(const LoggerPtr &logger, fmt::format_string<Args...> fmt,             \
       Args &&...args) -> NAME<Args...>;

_COLD_OUTPUT_IMPL(LogLevel::TRACE, TRACE)
_COLD_OUTPUT_IMPL(LogLevel::DEBUG, DEBUG)
_COLD_OUTPUT_IMPL(LogLevel::INFO, INFO)
_COLD_OUTPUT_IMPL(LogLevel::WARN, WARN)
_COLD_OUTPUT_IMPL(LogLevel::ERROR, ERROR)
_COLD_OUTPUT_IMPL(LogLevel::FATAL, FATAL)

#undef _COLD_OUTPUT_IMPL

}  // namespace Cold

#endif /* COLD_LOG_LOG */
