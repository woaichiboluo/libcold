#ifndef COLD_LOG_LOGCOMMON
#define COLD_LOG_LOGCOMMON

#include <source_location>
#include <string_view>

#include "cold/time/Time.h"
#include "third_party/fmt/include/fmt/format.h"

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

using LogBuffer = fmt::memory_buffer;

}  // namespace Cold::Base

#endif /* COLD_LOG_LOGCOMMON */
