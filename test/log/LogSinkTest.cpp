#include <initializer_list>
#include <memory>

#include "cold/log/Logger.h"
#include "cold/log/sinks/BasicFileSink.h"
#include "cold/log/sinks/LogSink.h"
#include "cold/log/sinks/StdoutSink.h"
using namespace Cold;

void TestOneLoggerMultiSinks() {
  Base::SinkPtr basicFileSink = std::make_shared<Base::BasicFileSink>(
      "LogSinkTest_OneLoggerMultiSinks.log");
  Base::SinkPtr stdoutSink = std::make_shared<Base::StdoutLogSink>();
  Base::SinkPtr stdoutSink1 = std::make_shared<Base::StdoutLogSink>();

  auto logger = std::make_shared<Base::Logger>(
      "LogSinkTest", std::initializer_list<Base::SinkPtr>{
                         basicFileSink, stdoutSink, stdoutSink1});
  for (int i = 0; i < 1000; ++i) {
    Base::Info(logger, "TestOneLoggerMultiSinks");
  }
}

void TestOneSinkMultiLoggers() {
  auto sink = std::make_shared<Base::BasicFileSink>(
      "LogSinkTest_OneSinkMultiLoggers.log");
  auto formatter =
      std::make_unique<Base::LogFormatter>("%T %L <%N:%t> %m %c [%b: %l] %n");
  Base::LogManager::Instance().AddLogger(
      std::make_shared<Base::Logger>("logger1", sink));
  Base::LogManager::Instance().AddLogger(
      std::make_shared<Base::Logger>("logger2", sink));
  Base::LogManager::Instance().AddLogger(
      std::make_shared<Base::Logger>("logger3", sink));
  Base::LogManager::Instance().AddLogger(
      std::make_shared<Base::Logger>("logger4", sink));
  Base::LogManager::Instance().GetLogger("logger1")->SetFormatter(
      std::move(formatter));
  auto logFunc = [](std::string name) {
    auto logger = Base::LogManager::Instance().GetLogger(name);
    for (int i = 0; i < 10000; ++i) {
      Base::Info(logger, "TestOneSinkMultiLoggers");
    }
  };
  std::vector<std::unique_ptr<Base::Thread>> threads;
  for (size_t i = 0; i < 4; ++i) {
    auto thread = std::make_unique<Base::Thread>(
        std::bind(logFunc, fmt::format("logger{}", i + 1)));
    thread->Start();
    threads.push_back(std::move(thread));
  }
  for (size_t i = 0; i < 4; ++i) {
    threads[i]->Join();
  }
  assert(Base::LogManager::Instance().RemoveLogger("logger1"));
  assert(Base::LogManager::Instance().RemoveLogger("logger2"));
  assert(Base::LogManager::Instance().RemoveLogger("logger3"));
  assert(Base::LogManager::Instance().RemoveLogger("logger4"));
}

void TestMultiSinksMultiLoggers() {
  // auto formatter = Base::MakeFormatter("%T %L <%N:%t> %m %c [%b: %l] %n");
  auto sink1 = std::make_shared<Base::BasicFileSink>(
      "LogSinkTest_TestMultiSinksMultiLoggers_1.log");
  auto sink2 = std::make_shared<Base::BasicFileSink>(
      "LogSinkTest_TestMultiSinksMultiLoggers_2.log");
  auto sink3 = std::make_shared<Base::StdoutLogSink>();
  auto sink4 = std::make_shared<Base::StdoutLogSink>();
  Base::LogManager::Instance().AddLogger(std::make_shared<Base::Logger>(
      "logger1",
      std::initializer_list<Base::SinkPtr>{sink1, sink2, sink3, sink4}));
  Base::LogManager::Instance().AddLogger(std::make_shared<Base::Logger>(
      "logger2",
      std::initializer_list<Base::SinkPtr>{sink1, sink2, sink3, sink4}));
  Base::LogManager::Instance().AddLogger(std::make_shared<Base::Logger>(
      "logger3",
      std::initializer_list<Base::SinkPtr>{sink1, sink2, sink3, sink4}));
  Base::LogManager::Instance().AddLogger(std::make_shared<Base::Logger>(
      "logger4",
      std::initializer_list<Base::SinkPtr>{sink1, sink2, sink3, sink4}));
  Base::LogManager::Instance().GetLogger("logger1")->SetPattern(
      "%T %L <%N:%t> %m %c [%b: %l] %n");
  // sink4->SetPattern("%T %C%L%K <%N:%t> %m %c [%b: %l] %n");
  auto logFunc = [](std::string name) {
    auto logger = Base::LogManager::Instance().GetLogger(name);
    for (int i = 0; i < 10000; ++i) {
      Base::Info(logger, "TestMultiSinkMultiLoggers");
    }
  };
  std::vector<std::unique_ptr<Base::Thread>> threads;
  for (size_t i = 0; i < 4; ++i) {
    auto thread = std::make_unique<Base::Thread>(
        std::bind(logFunc, fmt::format("logger{}", i + 1)));
    thread->Start();
    threads.push_back(std::move(thread));
  }
  for (size_t i = 0; i < 4; ++i) {
    threads[i]->Join();
  }
  assert(Base::LogManager::Instance().RemoveLogger("logger1"));
  assert(Base::LogManager::Instance().RemoveLogger("logger2"));
  assert(Base::LogManager::Instance().RemoveLogger("logger3"));
  assert(Base::LogManager::Instance().RemoveLogger("logger4"));
}

void TestResetMainLogger() {
  auto stdoutSink1 = std::make_shared<Base::StdoutLogSink>();
  auto stdoutSink2 = std::make_shared<Base::StdoutLogSink>();
  auto logger = std::make_shared<Base::Logger>(
      "logger", std::initializer_list<Base::SinkPtr>{stdoutSink1, stdoutSink2});
  Base::LogManager::Instance().SetMainLogger(logger);
  Base::INFO("Hello World!");
}

int main() {
  TestOneLoggerMultiSinks();
  TestOneSinkMultiLoggers();
  TestMultiSinksMultiLoggers();
  TestResetMainLogger();
}
