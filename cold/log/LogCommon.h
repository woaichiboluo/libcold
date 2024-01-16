#ifndef COLD_LOG_LOGCOMMON
#define COLD_LOG_LOGCOMMON

#include <source_location>
#include <string_view>
#include <utility>

#include "cold/time/Time.h"

namespace Cold::Base {

enum class LogLevel { TRACE, DEBUG, INFO, WARN, ERROR, FATAL, OFF };

struct LogMessage {
  LogLevel level;
  std::string_view threadId;
  std::string_view threadName;
  std::string_view loggerName;
  std::source_location location;
  std::string_view baseName;
  Time logTime;
  std::string_view logLine;
};

using LogBuffer = std::string;

}  // namespace Cold::Base

#endif /* COLD_LOG_LOGCOMMON */
