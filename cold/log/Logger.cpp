#include "cold/log/Logger.h"

#include <cstdlib>
#include <memory>
#include <unordered_map>

#include "cold/log/LogCommon.h"
#include "cold/log/LogSink.h"
#include "cold/log/StdoutSink.h"
#include "cold/thread/Lock.h"

using namespace Cold::Base;

void Logger::SinkIt(const LogMessage& message) {
  if (loggerLevel_ == LogLevel::OFF) return;

  assert(message.level < LogLevel::OFF);
  assert(loggerLevel_ <= LogLevel::FATAL);

  if (message.level >= loggerLevel_) {
    for (auto& sink : loggerSinks_) sink->Sink(message);
  }

  if (ShouldFlush(message)) Flush();

  if (message.level == LogLevel::FATAL) abort();
}

bool Logger::ShouldFlush(const LogMessage& message) const {
  return message.level != LogLevel::OFF && message.level >= flushLevel_.load();
}

void Logger::Flush() {
  for (auto& sink : loggerSinks_) {
    sink->Flush();
  }
}
namespace {
Mutex g_mutex;
LoggerPtr g_mainLogger GUARDED_BY(g_mutex) = std::make_shared<Logger>(
    "main-logger", std::make_unique<StdoutColorLogSink>());
std::unordered_map<std::string, LoggerPtr> g_loggerMap GUARDED_BY(g_mutex);
}  // namespace

namespace Cold::Base {
LoggerPtr GetMainLogger() {
  LockGuard guard(g_mutex);
  return g_mainLogger;
}

void SetMainLogger(LoggerPtr logger) {
  LockGuard guard(g_mutex);
  g_mainLogger = logger;
}

LoggerPtr GetLogger(const std::string& name) {
  LockGuard guard(g_mutex);
  auto it = g_loggerMap.find(name);
  return it == g_loggerMap.end() ? nullptr : it->second;
}

bool RemoveLogger(const std::string& name) {
  LockGuard guard(g_mutex);
  return g_loggerMap.erase(name);
}

void RegisterLogger(LoggerPtr logger) {
  LockGuard guard(g_mutex);
  g_loggerMap[logger->GetName()] = logger;
}
}  // namespace Cold::Base