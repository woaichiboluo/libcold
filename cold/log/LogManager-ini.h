#ifndef COLD_LOG_LOGMANAGER_INI
#define COLD_LOG_LOGMANAGER_INI

#include "Logger.h"
#include "logsinks/StdoutColorSink.h"

namespace Cold {

inline LogManager& LogManager::GetInstance() {
  static LogManager instance;
  return instance;
}

inline void LogManager::Add(LoggerPtr logger) {
  std::unique_lock lock(mutex_);
  loggers_[logger->GetName()] = logger;
}

inline LogManager::LogManager()
    : defaultLogger_(std::make_shared<Logger>(
          "main", std::make_shared<StdoutColorSink>())) {}

}  // namespace Cold

#endif /* COLD_LOG_LOGMANAGER_INI */
