#include "cold/log/Logger.h"

#include <strings.h>

#include <cassert>
#include <cstdlib>
#include <memory>

#include "cold/log/LogCommon.h"
#include "cold/log/LogSinkFactory.h"
#include "cold/thread/Lock.h"
#include "cold/util/Config.h"

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
  auto& config = Config::GetGloablDefaultConfig();
  // get config
  auto name = config.GetOrDefault("/logs/name", std::string("main-logger"));
  auto level = config.GetOrDefault("/logs/level", std::string("info"));
  auto flushLevel =
      config.GetOrDefault("/logs/level", std::string("flush-level"));
  auto loggerPattern = config.GetOrDefault("/logs/pattern", std::string(""));
  // set sinks
  if (config.Contains("/logs/sinks")) {
    assert(config.GetConfig("/logs/sinks").is_array());
    std::vector<SinkPtr> sinks;
    auto allSinksConfig = config.GetConfig("/logs/sinks");
    for (const auto& sinkConfig : allSinksConfig) {
      // get type and pattern
      auto type = sinkConfig["type"];
      assert(!type.empty());
      std::string pattern;
      if (sinkConfig.contains("pattern")) pattern = sinkConfig["pattern"];
      // get args
      std::vector<std::variant<int, bool, std::string>> args;
      for (int j = 1;; ++j) {
        auto argsPrefix = "arg" + std::to_string(j);
        if (!sinkConfig.contains(argsPrefix)) break;
        if (sinkConfig[argsPrefix].is_string()) {
          args.push_back(sinkConfig[argsPrefix].get<std::string>());
        } else if (sinkConfig[argsPrefix].is_boolean()) {
          args.push_back(sinkConfig[argsPrefix].get<bool>());
        } else {
          args.push_back(sinkConfig[argsPrefix].get<int>());
        }
      }
      // make logsink
      auto ptr = LogSinkFactory::MakeSink(type, pattern, args);
      // sink pattern but loggerPattern not empty use loggerPattern
      if (pattern.empty() && !loggerPattern.empty())
        ptr->SetPattern(loggerPattern);
      sinks.push_back(std::move(ptr));
    }
    mainLogger_ =
        std::make_shared<Logger>(std::move(name), sinks.begin(), sinks.end());
  } else {
    auto defaultSink = LogSinkFactory::MakeSink<StdoutColorSink>();
    if (!loggerPattern.empty()) defaultSink->SetPattern(loggerPattern);
    mainLogger_ = std::make_shared<Logger>(std::move(name), defaultSink);
  }
  // set levels
  const char* arr[7] = {"trace", "debug", "info", "warn",
                        "error", "fatal", "off"};
  for (size_t i = 0; i < 7; ++i) {
    if (strcasecmp(level.data(), arr[i]) == 0) {
      mainLogger_->SetLevel(static_cast<LogLevel>(i));
    }
    if (strcasecmp(flushLevel.data(), arr[i])) {
      mainLogger_->SetFlushLevel(static_cast<LogLevel>(i));
    }
  }
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