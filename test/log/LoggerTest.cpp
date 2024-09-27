#include "cold/Cold.h"

using namespace Cold;

void TraceLevel() {
  auto logger = LogManager::GetInstance().GetDefaultRaw();
  logger->SetLevel(LogLevel::TRACE);
  TRACE("Level at Trace");
  DEBUG("Level at Trace");
  INFO("Level at Trace");
  WARN("Level at Trace");
  ERROR("Level at Trace");
}

void DebugLevel() {
  auto logger = LogManager::GetInstance().GetDefaultRaw();
  logger->SetLevel(LogLevel::DEBUG);
  TRACE("Level at Debug");
  DEBUG("Level at Debug");
  INFO("Level at Debug");
  WARN("Level at Debug");
  ERROR("Level at Debug");
}

void InfoLevel() {
  auto logger = LogManager::GetInstance().GetDefaultRaw();
  logger->SetLevel(LogLevel::INFO);
  TRACE(logger, "Level at Info");
  DEBUG(logger, "Level at Info");
  INFO(logger, "Level at Info");
  WARN(logger, "Level at Info");
  ERROR(logger, "Level at Info");
}

void WarnLevel() {
  auto logger = LogManager::GetInstance().GetDefaultRaw();
  logger->SetLevel(LogLevel::WARN);
  TRACE(logger, "Level at Warn");
  DEBUG(logger, "Level at Warn");
  INFO(logger, "Level at Warn");
  WARN(logger, "Level at Warn");
  ERROR(logger, "Level at Warn");
}

void ErrorLevel() {
  auto logger = LogManager::GetInstance().GetDefault();
  logger->SetLevel(LogLevel::ERROR);
  TRACE("Level at Error");
  DEBUG("Level at Error");
  INFO("Level at Error");
  WARN("Level at Error");
  ERROR("Level at Error");
}

void CloseAll() {
  auto logger = LogManager::GetInstance().GetDefaultRaw();
  logger->SetLevel(LogLevel::OFF);
  TRACE(logger, "Level at Off");
  DEBUG(logger, "Level at Off");
  INFO(logger, "Level at Off");
  WARN(logger, "Level at Off");
  ERROR(logger, "Level at Off");
}

int main() {
  TraceLevel();
  DebugLevel();
  InfoLevel();
  WarnLevel();
  ErrorLevel();
  CloseAll();
  LogManager::GetInstance().GetDefaultRaw()->SetLevel(LogLevel::INFO);
  FATAL("do abort");
}