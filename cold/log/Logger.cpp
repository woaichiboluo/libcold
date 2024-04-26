#include "cold/log/Logger.h"

#include <cassert>
#include <cstdlib>
#include <memory>

#include "cold/log/LogCommon.h"
#include "cold/log/sinks/StdoutSink.h"
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

void Logger::SetPattern(std::string_view pattern) {
  for (auto& sink : loggerSinks_) {
    sink->SetPattern(pattern);
  }
}

void Logger::SetFormatter(LogFormatterPtr formatter) {
  for (auto& sink : loggerSinks_) {
    sink->SetFormatter(formatter->Clone());
  }
}

LogManager::LogManager() {
  auto defaultSink = std::make_shared<StdoutLogSink>();
  mainLogger_ = std::make_shared<Logger>("main", defaultSink);
}

LogManager& LogManager::Instance() {
  static LogManager manager;
  return manager;
}

LoggerPtr LogManager::GetMainLogger() {
  LockGuard guard(mutex_);
  return mainLogger_;
}

void LogManager::SetMainLogger(LoggerPtr logger) {
  LockGuard guard(mutex_);
  mainLogger_ = logger;
}

Logger* LogManager::GetMainLoggerRaw() { return mainLogger_.get(); }

LoggerPtr LogManager::GetLogger(const std::string& loggerName) {
  LockGuard guard(mutex_);
  auto it = loggersMap_.find(loggerName);
  return it == loggersMap_.end() ? nullptr : it->second;
}

bool LogManager::RemoveLogger(const std::string& loggerName) {
  LockGuard guard(mutex_);
  return loggersMap_.erase(loggerName);
}

void LogManager::AddLogger(LoggerPtr logger) {
  LockGuard guard(mutex_);
  loggersMap_[logger->GetName()] = logger;
}