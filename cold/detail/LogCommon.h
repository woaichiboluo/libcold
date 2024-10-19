#ifndef COLD_DETAIL_LOGCOMMON
#define COLD_DETAIL_LOGCOMMON

#include <source_location>

#include "../time/Time.h"
#include "fmt.h"

namespace Cold {

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

}  // namespace Cold

#endif /* COLD_DETAIL_LOGCOMMON */
