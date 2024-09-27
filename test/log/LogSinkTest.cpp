#include "cold/Cold.h"
#include "cold/log/logsinks/BasicFileSink.h"
#include "cold/log/logsinks/NullSink.h"
#include "cold/log/logsinks/StdoutColorSink.h"
#include "cold/log/logsinks/StdoutSink.h"

using namespace Cold;

void TestOneLoggerMultiSinks() {
  auto basicFileSink = LogSinkFactory::Create<BasicFileSink>(
      "LogSinkTest_OneLoggerMultiSinks.log");
  auto stdoutSink = LogSinkFactory::Create<StdoutSink>();
  auto stdoutColorSink = LogSinkFactory::Create<StdoutColorSink>();

  auto logger = LogFactory::MakeLogger(
      "LogSinkTest", {basicFileSink, stdoutSink, stdoutColorSink});

  for (int i = 0; i < 1000; ++i) {
    INFO(logger, "TestOneLoggerMultiSinks");
  }
}

void TestOneSinkMultiLoggers() {
  auto sink = LogSinkFactory::Create<BasicFileSink>(
      "LogSinkTest_OneSinkMultiLoggers.log");
  auto formatter =
      std::make_unique<LogFormatter>("%T %L <%N:%t> %m %c [%b: %l]%n");
  LogFactory::MakeLogger("logger1", {sink});
  LogFactory::MakeLogger("logger2", {sink});
  LogFactory::MakeLogger("logger3", {sink});
  LogFactory::MakeLogger("logger4", {sink});
  LogManager::GetInstance().GetLogger("logger1")->SetFormatter(
      formatter->Clone());
  auto logFunc = [](std::string name) {
    auto logger = LogManager::GetInstance().GetLogger(name);
    for (int i = 0; i < 10000; ++i) {
      INFO(logger, "TestOneSinkMultiLoggers{}", 1);
    }
  };
  std::vector<std::unique_ptr<Thread>> threads;
  for (size_t i = 0; i < 4; ++i) {
    auto thread = std::make_unique<Thread>(
        std::bind(logFunc, fmt::format("logger{}", i + 1)));
    thread->Start();
    threads.push_back(std::move(thread));
  }
  for (size_t i = 0; i < 4; ++i) {
    threads[i]->Join();
  }
  assert(LogManager::GetInstance().Drop("logger1"));
  assert(LogManager::GetInstance().Drop("logger2"));
  assert(LogManager::GetInstance().Drop("logger3"));
  assert(LogManager::GetInstance().Drop("logger4"));
}

void TestMultiSinksMultiLoggers() {
  // auto formatter = Base::MakeFormatter("%T %L <%N:%t> %m %c [%b: %l] %n");
  auto sink1 = LogSinkFactory::Create<BasicFileSink>(
      "LogSinkTest_TestMultiSinksMultiLoggers_1.log");
  auto sink2 = LogSinkFactory::Create<BasicFileSink>(
      "LogSinkTest_TestMultiSinksMultiLoggers_2.log");
  auto sink3 = LogSinkFactory::Create<StdoutSink>();
  auto sink4 = LogSinkFactory::Create<StdoutColorSink>();
  LogFactory::MakeLogger("logger1", {sink1, sink2, sink3, sink4});
  LogFactory::MakeLogger("logger2", {sink1, sink2, sink3, sink4});
  LogFactory::MakeLogger("logger3", {sink1, sink2, sink3, sink4});
  LogFactory::MakeLogger("logger4", {sink1, sink2, sink3, sink4});
  LogManager::GetInstance().GetLogger("logger1")->SetPattern(
      "%T %L <%N:%t> %m %c [%b: %l] %n");
  sink4->SetPattern("%T %C%L%E <%N:%t> %m %c [%b: %l] %n");
  auto logFunc = [](std::string name) {
    auto logger = LogManager::GetInstance().GetLoggerRaw(name);
    for (int i = 0; i < 10000; ++i) {
      INFO(logger, "TestMultiSinkMultiLoggers");
    }
  };
  std::vector<std::unique_ptr<Thread>> threads;
  for (size_t i = 0; i < 4; ++i) {
    auto thread = std::make_unique<Thread>(
        std::bind(logFunc, fmt::format("logger{}", i + 1)));
    thread->Start();
    threads.push_back(std::move(thread));
  }
  for (size_t i = 0; i < 4; ++i) {
    threads[i]->Join();
  }
  assert(LogManager::GetInstance().Drop("logger1"));
  assert(LogManager::GetInstance().Drop("logger2"));
  assert(LogManager::GetInstance().Drop("logger3"));
  assert(LogManager::GetInstance().Drop("logger4"));
}

void TestResetMainLogger() {
  auto stdoutSink1 = LogSinkFactory::Create<StdoutSink>();
  auto stdoutSink2 = LogSinkFactory::Create<StdoutColorSink>();
  auto logger = LogFactory::MakeLogger("logger", {stdoutSink1, stdoutSink2});
  LogManager::GetInstance().SetDefault(logger);
  INFO("Hello World!");
}

void TestLoggerFactory() {
  auto logger1 = MakeFileLogger("basicfilelogger", "TestLoggerFactory.log");
  auto l = LogManager::GetInstance().GetLogger("basicfilelogger");
  auto logger2 = MakeNullLogger("nulllog");
  auto logger3 = MakeStdoutLogger("stdoutlogger");
  auto logger4 = MakeStdoutColorLogger("stdoutcolorlogger");
  logger1->SetLevel(LogLevel::TRACE);
  logger2->SetLevel(LogLevel::TRACE);
  logger3->SetLevel(LogLevel::TRACE);
  logger4->SetLevel(LogLevel::TRACE);
  TRACE(LogManager::GetInstance().GetLogger("basicfilelogger"), "{}",
        "basicfilelogger");
  DEBUG(LogManager::GetInstance().GetLogger("basicfilelogger"), "{}",
        "basicfilelogger");
  INFO(LogManager::GetInstance().GetLogger("basicfilelogger"), "{}",
       "basicfilelogger");
  WARN(LogManager::GetInstance().GetLogger("basicfilelogger"), "{}",
       "basicfilelogger");
  ERROR(LogManager::GetInstance().GetLogger("basicfilelogger"), "{}",
        "basicfilelogger");
  TRACE(LogManager::GetInstance().GetLogger("nulllog"), "{}", "nulllog");
  DEBUG(LogManager::GetInstance().GetLogger("nulllog"), "{}", "nulllog");
  INFO(LogManager::GetInstance().GetLogger("nulllog"), "{}", "nulllog");
  WARN(LogManager::GetInstance().GetLogger("nulllog"), "{}", "nulllog");
  ERROR(LogManager::GetInstance().GetLogger("nulllog"), "{}", "nulllog");
  TRACE(LogManager::GetInstance().GetLogger("stdoutlogger"), "{}",
        "stdoutlogger");
  DEBUG(LogManager::GetInstance().GetLogger("stdoutlogger"), "{}",
        "stdoutlogger");
  INFO(LogManager::GetInstance().GetLogger("stdoutlogger"), "{}",
       "stdoutlogger");
  WARN(LogManager::GetInstance().GetLogger("stdoutlogger"), "{}",
       "stdoutlogger");
  ERROR(LogManager::GetInstance().GetLogger("stdoutlogger"), "{}",
        "stdoutlogger");
  TRACE(LogManager::GetInstance().GetLogger("stdoutcolorlogger"), "{}",
        "stdoutcolorlogger");
  DEBUG(LogManager::GetInstance().GetLogger("stdoutcolorlogger"), "{}",
        "stdoutcolorlogger");
  INFO(LogManager::GetInstance().GetLogger("stdoutcolorlogger"), "{}",
       "stdoutcolorlogger");
  WARN(LogManager::GetInstance().GetLogger("stdoutcolorlogger"), "{}",
       "stdoutcolorlogger");
  ERROR(LogManager::GetInstance().GetLogger("stdoutcolorlogger"), "{}",
        "stdoutcolorlogger");
  assert(LogManager::GetInstance().Drop("basicfilelogger"));
  assert(LogManager::GetInstance().Drop("nulllog"));
  assert(LogManager::GetInstance().Drop("stdoutlogger"));
  assert(LogManager::GetInstance().Drop("stdoutcolorlogger"));
}

int main() {
  TestOneLoggerMultiSinks();
  TestOneSinkMultiLoggers();
  TestMultiSinksMultiLoggers();
  TestResetMainLogger();
  TestLoggerFactory();
}
