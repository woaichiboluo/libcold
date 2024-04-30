#include "cold/log/Logger.h"

#include <cassert>
#include <cstdlib>
#include <memory>

#include "cold/log/LogCommon.h"
#include "cold/log/sinks/StdoutSink.h"
#include "cold/thread/Lock.h"

using namespace Cold;

void Base::Logger::SinkIt(const LogMessage& message) {
  if (loggerLevel_ == LogLevel::OFF) return;

  assert(message.level < LogLevel::OFF);
  assert(loggerLevel_ <= LogLevel::FATAL);

  if (message.level >= loggerLevel_) {
    for (auto& sink : loggerSinks_) sink->Sink(message);
  }

  if (ShouldFlush(message)) Flush();

  if (message.level == LogLevel::FATAL) abort();
}

bool Base::Logger::ShouldFlush(const LogMessage& message) const {
  return message.level != LogLevel::OFF && message.level >= flushLevel_.load();
}

void Base::Logger::Flush() {
  for (auto& sink : loggerSinks_) {
    sink->Flush();
  }
}

void Base::Logger::SetPattern(std::string_view pattern) {
  for (auto& sink : loggerSinks_) {
    sink->SetPattern(pattern);
  }
}

void Base::Logger::SetFormatter(LogFormatterPtr formatter) {
  for (auto& sink : loggerSinks_) {
    sink->SetFormatter(formatter->Clone());
  }
}

Base::LogManager::LogManager() {
  auto defaultSink = std::make_shared<StdoutLogSink>();
  mainLogger_ = std::make_shared<Logger>("main", defaultSink);
}

Base::LogManager& Base::LogManager::Instance() {
  static LogManager manager;
  return manager;
}

Base::LoggerPtr Base::LogManager::GetMainLogger() {
  LockGuard guard(mutex_);
  return mainLogger_;
}

void Base::LogManager::SetMainLogger(LoggerPtr logger) {
  LockGuard guard(mutex_);
  mainLogger_ = logger;
}

Base::Logger* Base::LogManager::GetMainLoggerRaw() { return mainLogger_.get(); }

Base::LoggerPtr Base::LogManager::GetLogger(const std::string& loggerName) {
  LockGuard guard(mutex_);
  auto it = loggersMap_.find(loggerName);
  return it == loggersMap_.end() ? nullptr : it->second;
}

bool Base::LogManager::RemoveLogger(const std::string& loggerName) {
  LockGuard guard(mutex_);
  return loggersMap_.erase(loggerName);
}

void Base::LogManager::AddLogger(LoggerPtr logger) {
  LockGuard guard(mutex_);
  loggersMap_[logger->GetName()] = logger;
}