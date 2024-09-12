#ifndef COLD_LOG
#define COLD_LOG

#include "cold/log/LogManager-ini.h"
#include "cold/log/Logger.h"

namespace Cold {

using LoggerPtr = std::shared_ptr<Logger>;
#define _COLD_OUTPUT_IMPL(LOGLEVEL, NAME)                                    \
  template <typename... Args>                                                \
  struct NAME {                                                              \
    constexpr NAME(const LoggerPtr &logger, fmt::format_string<Args...> fmt, \
                   Args &&...args, detail::LocationWrapper wrap = {}) {      \
      logger->Log(LOGLEVEL, wrap, fmt, std::forward<Args>(args)...);         \
    }                                                                        \
    constexpr NAME(Logger *logger, fmt::format_string<Args...> fmt,          \
                   Args &&...args, detail::LocationWrapper wrap = {}) {      \
      logger->Log(LOGLEVEL, wrap, fmt, std::forward<Args>(args)...);         \
    }                                                                        \
    constexpr NAME(fmt::format_string<Args...> fmt, Args &&...args,          \
                   detail::LocationWrapper wrap = {}) {                      \
      LogManager::GetInstance().GetDefaultRaw()->Log(                        \
          LOGLEVEL, wrap, fmt, std::forward<Args>(args)...);                 \
    }                                                                        \
  };                                                                         \
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

#endif /* COLD_LOG */
