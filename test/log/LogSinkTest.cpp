#include <initializer_list>
#include <memory>

#include "cold/log/LogCommon.h"
#include "cold/log/LogSinkFactory.h"
#include "cold/log/Logger.h"
#include "cold/log/sinks/BasicFileSink.h"
#include "cold/log/sinks/NullSink.h"
#include "cold/log/sinks/StdoutColorSink.h"
#include "cold/log/sinks/StdoutSink.h"

using namespace Cold;

void TestOneLoggerMultiSinks() {
  auto basicFileSink = Base::LogSinkFactory::MakeSink<Base::BasicFileSink>(
      "LogSinkTest_OneLoggerMultiSinks.log");
  auto stdoutSink = Base::LogSinkFactory::MakeSink<Base::StdoutSink>();
  auto stdoutColorSink =
      Base::LogSinkFactory::MakeSink<Base::StdoutColorSink>();

  auto logger = std::make_shared<Base::Logger>(
      "LogSinkTest", std::initializer_list<Base::SinkPtr>{
                         basicFileSink, stdoutSink, stdoutColorSink});
  for (int i = 0; i < 1000; ++i) {
    Base::Info(logger, "TestOneLoggerMultiSinks");
  }
}

void TestOneSinkMultiLoggers() {
  auto sink = Base::LogSinkFactory::MakeSink<Base::BasicFileSink>(
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
      Base::Info(logger, "TestOneSinkMultiLoggers{}", 1);
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
  auto sink1 = Base::LogSinkFactory::MakeSink<Base::BasicFileSink>(
      "LogSinkTest_TestMultiSinksMultiLoggers_1.log");
  auto sink2 = Base::LogSinkFactory::MakeSink<Base::BasicFileSink>(
      "LogSinkTest_TestMultiSinksMultiLoggers_2.log");
  auto sink3 = Base::LogSinkFactory::MakeSink<Base::StdoutSink>();
  auto sink4 = Base::LogSinkFactory::MakeSink<Base::StdoutColorSink>();
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
  auto stdoutSink1 = Base::LogSinkFactory::MakeSink<Base::StdoutSink>();
  auto stdoutSink2 = Base::LogSinkFactory::MakeSink<Base::StdoutColorSink>();
  auto logger = std::make_shared<Base::Logger>(
      "logger", std::initializer_list<Base::SinkPtr>{stdoutSink1, stdoutSink2});
  Base::LogManager::Instance().SetMainLogger(logger);
  Base::INFO("Hello World!");
}

void TestLoggerFactory() {
  auto logger1 =
      Base::MakeBasicFileLogger("basicfilelogger", "TestLoggerFactory.log");
  auto l = Base::LogManager::Instance().GetLogger("basicfilelogger");
  auto logger2 = Base::MakeNullSink("nulllog");
  auto logger3 = Base::MakeStdoutLogger("stdoutlogger");
  auto logger4 = Base::MakeStdoutColorLogger("stdoutcolorlogger");
  logger1->SetLevel(Base::LogLevel::TRACE);
  logger2->SetLevel(Base::LogLevel::TRACE);
  logger3->SetLevel(Base::LogLevel::TRACE);
  logger4->SetLevel(Base::LogLevel::TRACE);
  Base::Trace(Base::LogManager::Instance().GetLogger("basicfilelogger"), "{}",
              "basicfilelogger");
  Base::Debug(Base::LogManager::Instance().GetLogger("basicfilelogger"), "{}",
              "basicfilelogger");
  Base::Info(Base::LogManager::Instance().GetLogger("basicfilelogger"), "{}",
             "basicfilelogger");
  Base::Warn(Base::LogManager::Instance().GetLogger("basicfilelogger"), "{}",
             "basicfilelogger");
  Base::Error(Base::LogManager::Instance().GetLogger("basicfilelogger"), "{}",
              "basicfilelogger");
  Base::Trace(Base::LogManager::Instance().GetLogger("nulllog"), "{}",
              "nulllog");
  Base::Debug(Base::LogManager::Instance().GetLogger("nulllog"), "{}",
              "nulllog");
  Base::Info(Base::LogManager::Instance().GetLogger("nulllog"), "{}",
             "nulllog");
  Base::Warn(Base::LogManager::Instance().GetLogger("nulllog"), "{}",
             "nulllog");
  Base::Error(Base::LogManager::Instance().GetLogger("nulllog"), "{}",
              "nulllog");
  Base::Trace(Base::LogManager::Instance().GetLogger("stdoutlogger"), "{}",
              "stdoutlogger");
  Base::Debug(Base::LogManager::Instance().GetLogger("stdoutlogger"), "{}",
              "stdoutlogger");
  Base::Info(Base::LogManager::Instance().GetLogger("stdoutlogger"), "{}",
             "stdoutlogger");
  Base::Warn(Base::LogManager::Instance().GetLogger("stdoutlogger"), "{}",
             "stdoutlogger");
  Base::Error(Base::LogManager::Instance().GetLogger("stdoutlogger"), "{}",
              "stdoutlogger");
  Base::Trace(Base::LogManager::Instance().GetLogger("stdoutcolorlogger"), "{}",
              "stdoutcolorlogger");
  Base::Debug(Base::LogManager::Instance().GetLogger("stdoutcolorlogger"), "{}",
              "stdoutcolorlogger");
  Base::Info(Base::LogManager::Instance().GetLogger("stdoutcolorlogger"), "{}",
             "stdoutcolorlogger");
  Base::Warn(Base::LogManager::Instance().GetLogger("stdoutcolorlogger"), "{}",
             "stdoutcolorlogger");
  Base::Error(Base::LogManager::Instance().GetLogger("stdoutcolorlogger"), "{}",
              "stdoutcolorlogger");
  assert(Base::LogManager::Instance().RemoveLogger("basicfilelogger"));
  assert(Base::LogManager::Instance().RemoveLogger("nulllog"));
  assert(Base::LogManager::Instance().RemoveLogger("stdoutlogger"));
  assert(Base::LogManager::Instance().RemoveLogger("stdoutcolorlogger"));
}

int main() {
  TestOneLoggerMultiSinks();
  TestOneSinkMultiLoggers();
  TestMultiSinksMultiLoggers();
  TestResetMainLogger();
  TestLoggerFactory();
}
