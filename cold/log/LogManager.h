#ifndef COLD_LOG_LOGMANAGER
#define COLD_LOG_LOGMANAGER

#include <memory>
#include <mutex>
#include <unordered_map>

#include "Logger.h"

namespace Cold {

class Logger;

class LogManager {
 public:
  using LoggerPtr = std::shared_ptr<Logger>;

  LogManager(const LogManager&) = delete;
  LogManager& operator=(const LogManager&) = delete;

  static LogManager& GetInstance() {
    static LogManager instance;
    return instance;
  }
  void SetDefault(LoggerPtr logger) {
    std::unique_lock lock(mutex_);
    defaultLogger_ = logger;
  }

  // thread unsafe
  Logger* GetDefaultRaw() { return defaultLogger_.get(); }
  // thread safe
  LoggerPtr GetDefault() {
    std::unique_lock lock(mutex_);
    return defaultLogger_;
  }

  void Add(LoggerPtr logger) {
    std::unique_lock lock(mutex_);
    loggers_[logger->GetName()] = logger;
  }

  bool Drop(const std::string& name) {
    std::unique_lock lock(mutex_);
    return loggers_.erase(name);
  }

  // thread unsafe
  Logger* GetLoggerRaw(const std::string& name) {
    auto it = loggers_.find(name);
    return it == loggers_.end() ? nullptr : it->second.get();
  }

  // thread safe
  LoggerPtr GetLogger(const std::string& name) {
    std::unique_lock lock(mutex_);
    auto it = loggers_.find(name);
    return it == loggers_.end() ? nullptr : it->second;
  }

 private:
  LogManager();
  ~LogManager() = default;

  std::mutex mutex_;
  std::shared_ptr<Logger> defaultLogger_;
  std::unordered_map<std::string, std::shared_ptr<Logger>> loggers_;
};

}  // namespace Cold

#endif /* COLD_LOG_LOGMANAGER */
