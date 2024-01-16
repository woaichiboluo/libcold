#include "cold/log/Logger.h"

#include <cstdlib>

#include "cold/log/LogCommon.h"
#include "cold/log/LogSink.h"

using namespace Cold::Base;

void Logger::SinkIt(const LogMessage& message) {
  if (loggerLevel_ == LogLevel::OFF) return;

  assert(message.level < LogLevel::OFF);
  assert(loggerLevel_ <= LogLevel::FATAL);

  if (message.level <= loggerLevel_) {
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