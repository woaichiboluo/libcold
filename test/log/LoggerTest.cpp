#include "cold/log/LogCommon.h"
#include "cold/log/Logger.h"

using namespace Cold;

void TraceLevel() {
  auto logger = Base::LogManager::Instance().GetMainLoggerRaw();
  logger->SetLevel(Base::LogLevel::TRACE);
  Base::TRACE("Level at Trace");
  Base::DEBUG("Level at Trace");
  Base::INFO("Level at Trace");
  Base::WARN("Level at Trace");
  Base::ERROR("Level at Trace");
}

void DebugLevel() {
  auto logger = Base::LogManager::Instance().GetMainLogger();
  logger->SetLevel(Base::LogLevel::DEBUG);
  Base::TRACE("Level at Debug");
  Base::DEBUG("Level at Debug");
  Base::INFO("Level at Debug");
  Base::WARN("Level at Debug");
  Base::ERROR("Level at Debug");
}

void InfoLevel() {
  auto logger = Base::LogManager::Instance().GetMainLogger();
  logger->SetLevel(Base::LogLevel::INFO);
  Base::Trace(logger, "Level at Info");
  Base::Debug(logger, "Level at Info");
  Base::Info(logger, "Level at Info");
  Base::Warn(logger, "Level at Info");
  Base::Error(logger, "Level at Info");
}

void WarnLevel() {
  auto logger = Base::LogManager::Instance().GetMainLogger();
  logger->SetLevel(Base::LogLevel::WARN);
  Base::Trace(logger, "Level at Warn");
  Base::Debug(logger, "Level at Warn");
  Base::Info(logger, "Level at Warn");
  Base::Warn(logger, "Level at Warn");
  Base::Error(logger, "Level at Warn");
}

void ErrorLevel() {
  auto logger = Base::LogManager::Instance().GetMainLoggerRaw();
  logger->SetLevel(Base::LogLevel::ERROR);
  Base::TRACE("Level at Error");
  Base::DEBUG("Level at Error");
  Base::INFO("Level at Error");
  Base::WARN("Level at Error");
  Base::ERROR("Level at Error");
}

void CloseAll() {
  auto logger = Base::LogManager::Instance().GetMainLogger();
  logger->SetLevel(Base::LogLevel::OFF);
  Base::Trace(logger, "Level at Off");
  Base::Debug(logger, "Level at Off");
  Base::Info(logger, "Level at Off");
  Base::Warn(logger, "Level at Off");
  Base::Error(logger, "Level at Off");
}

int main() {
  TraceLevel();
  DebugLevel();
  InfoLevel();
  WarnLevel();
  ErrorLevel();
  CloseAll();
  Base::LogManager::Instance().GetMainLoggerRaw()->SetLevel(
      Base::LogLevel::INFO);
  Base::FATAL("do abort");
}