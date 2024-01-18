#include <memory>

#include "cold/log/BasicFileSink.h"
#include "cold/log/LogFormatter.h"
#include "cold/log/Logger.h"
#include "cold/log/StdoutSink.h"
#include "cold/thread/Thread.h"
#include "third_party/fmt/include/fmt/format.h"

using namespace Cold;

void TestOneLoggerMultiSinks() {
  Base::SinkPtr basicFileSink = Base::MakeSink<Base::BasicFileSink>(
      "LogSinkTest_OneLoggerMultiSinks.log");
  Base::SinkPtr stdoutSink = Base::MakeSink<Base::StdoutLogSink>();
  Base::SinkPtr stdoutColorSink = Base::MakeSink<Base::StdoutColorLogSink>();

  auto logger = Cold::Base::MakeLogger(
      "LogSinkTest", {basicFileSink, stdoutSink, stdoutColorSink});
  for (int i = 0; i < 1000; ++i) {
    LOG_INFO(logger, "TestOneLoggerMultiSinks");
  }
}

void TestOneSinkMultiLoggers() {
  auto sink = Base::MakeSink<Base::BasicFileSink>(
      "LogSinkTest_OneSinkMultiLoggers.log");
  auto formatter = Base::MakeFormatter("%T %L <%N:%t> %m %c [%b: %l] %n");
  sink->SetFormatter(std::move(formatter));
  Base::RegisterLogger(Base::MakeLogger("logger1", sink));
  Base::RegisterLogger(Base::MakeLogger("logger2", sink));
  Base::RegisterLogger(Base::MakeLogger("logger3", sink));
  Base::RegisterLogger(Base::MakeLogger("logger4", sink));
  auto logFunc = [](std::string name) {
    auto logger = Base::GetLogger(name);
    for (int i = 0; i < 10000; ++i) {
      LOG_INFO(logger, "TestOneSinkMultiLoggers");
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
  assert(Base::RemoveLogger("logger1"));
  assert(Base::RemoveLogger("logger2"));
  assert(Base::RemoveLogger("logger3"));
  assert(Base::RemoveLogger("logger4"));
}

void TestMultiSinksMultiLoggers() {
  auto formatter = Base::MakeFormatter("%T %L <%N:%t> %m %c [%b: %l] %n");
  auto sink1 = Base::MakeSink<Base::BasicFileSink>(
      formatter->Clone(), "LogSinkTest_TestMultiSinksMultiLoggers_1.log");
  auto sink2 = Base::MakeSink<Base::BasicFileSink>(
      formatter->Clone(), "LogSinkTest_TestMultiSinksMultiLoggers_2.log");
  auto sink3 = Base::MakeSink<Base::StdoutLogSink>(formatter->Clone());
  formatter->AddFlag(
      'C', std::make_unique<Base::StdoutColorLogSink::CustomColorFlag>());
  formatter->AddFlag(
      'K', std::make_unique<Base::StdoutColorLogSink::ColorResetFlag>());
  formatter->SetPattern("%T %C%L%K <%N:%t> %m %c [%b: %l] %n");
  auto sink4 = Base::MakeSink<Base::StdoutColorLogSink>(formatter->Clone());
  Base::RegisterLogger(
      Base::MakeLogger("logger1", {sink1, sink2, sink3, sink4}));
  Base::RegisterLogger(
      Base::MakeLogger("logger2", {sink1, sink2, sink3, sink4}));
  Base::RegisterLogger(
      Base::MakeLogger("logger3", {sink1, sink2, sink3, sink4}));
  Base::RegisterLogger(
      Base::MakeLogger("logger4", {sink1, sink2, sink3, sink4}));
  auto logFunc = [](std::string name) {
    auto logger = Base::GetLogger(name);
    for (int i = 0; i < 10000; ++i) {
      LOG_INFO(logger, "TestOneSinkMultiLoggers");
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
  assert(Base::RemoveLogger("logger1"));
  assert(Base::RemoveLogger("logger2"));
  assert(Base::RemoveLogger("logger3"));
  assert(Base::RemoveLogger("logger4"));
}

void TestResetMainLogger() {
  auto stdoutSink = Base::MakeSink<Base::StdoutLogSink>();
  auto stdoutColorSink = Base::MakeSink<Base::StdoutColorLogSink>();
  auto logger = Base::MakeLogger("logger", {stdoutSink, stdoutColorSink});
  Base::SetMainLogger(logger);
  LOG_INFO(Base::GetMainLogger(), "hello world!");
}

int main() {
  TestOneLoggerMultiSinks();
  TestOneSinkMultiLoggers();
  TestMultiSinksMultiLoggers();
  TestResetMainLogger();
}
