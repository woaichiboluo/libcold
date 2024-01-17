#include "cold/log/LogCommon.h"
#include "cold/log/Logger.h"

using namespace Cold;

void TraceLevel() {
  auto logger = Base::GetMainLogger();
  logger->SetLevel(Base::LogLevel::TRACE);
  LOG_TRACE(logger, "Level at Trace");
  LOG_DEBUG(logger, "Level at Trace");
  LOG_INFO(logger, "Level at Trace");
  LOG_WARN(logger, "Level at Trace");
  LOG_ERROR(logger, "Level at Trace");
}

void DebugLevel() {
  auto logger = Base::GetMainLogger();
  logger->SetLevel(Base::LogLevel::DEBUG);
  LOG_TRACE(logger, "Level at Debug");
  LOG_DEBUG(logger, "Level at Debug");
  LOG_INFO(logger, "Level at Debug");
  LOG_WARN(logger, "Level at Debug");
  LOG_ERROR(logger, "Level at Debug");
}

void InfoLevel() {
  auto logger = Base::GetMainLogger();
  logger->SetLevel(Base::LogLevel::INFO);
  LOG_TRACE(logger, "Level at Info");
  LOG_DEBUG(logger, "Level at Info");
  LOG_INFO(logger, "Level at Info");
  LOG_WARN(logger, "Level at Info");
  LOG_ERROR(logger, "Level at Info");
}

void WarnLevel() {
  auto logger = Base::GetMainLogger();
  logger->SetLevel(Base::LogLevel::WARN);
  LOG_TRACE(logger, "Level at Warn");
  LOG_DEBUG(logger, "Level at Warn");
  LOG_INFO(logger, "Level at Warn");
  LOG_WARN(logger, "Level at Warn");
  LOG_ERROR(logger, "Level at Warn");
}

void ErrorLevel() {
  auto logger = Base::GetMainLogger();
  logger->SetLevel(Base::LogLevel::WARN);
  LOG_TRACE(logger, "Level at Error");
  LOG_DEBUG(logger, "Level at Error");
  LOG_INFO(logger, "Level at Error");
  LOG_WARN(logger, "Level at Error");
  LOG_ERROR(logger, "Level at Error");
}

void CloseAll() {
  auto logger = Base::GetMainLogger();
  logger->SetLevel(Base::LogLevel::OFF);
  LOG_TRACE(logger, "Level at OFF");
  LOG_DEBUG(logger, "Level at OFF");
  LOG_INFO(logger, "Level at OFF");
  LOG_WARN(logger, "Level at OFF");
  LOG_ERROR(logger, "Level at OFF");
}

int main() {
  TraceLevel();
  DebugLevel();
  InfoLevel();
  WarnLevel();
  ErrorLevel();
  CloseAll();
  auto logger = Base::GetMainLogger();
  LOG_FATAL(logger, "no output but aborted");
}