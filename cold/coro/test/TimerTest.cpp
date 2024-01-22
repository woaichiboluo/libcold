#include <chrono>

#include "cold/coro/IoContext.h"
#include "cold/coro/Timer.h"
#include "cold/log/LogCommon.h"
#include "cold/log/Logger.h"

using namespace Cold::Base;

namespace {
auto g_logger = GetMainLogger();
}

void TestBasicSteadyTimerUsage() {
  IoContext ioContext;
  SteadyTimer timer(ioContext);
  LOG_INFO(g_logger, "Start Timer Event");
  timer.ExpiresAfter(std::chrono::milliseconds(200));
  timer.AsyncAwait([](IoContext& context) -> Task<void> {
    context.Stop();
    LOG_INFO(g_logger, "Stop the IoContext");
    co_return;
  }(ioContext));
  ioContext.Start();
};

void TestUseSteadyTimerRepeat() {
  IoContext ioContext;
  SteadyTimer timer(ioContext);
  int count = 0;
  std::function<Task<>()> taskGenerator = [&]() -> Task<> {
    LOG_INFO(g_logger, "SteadyTime Timeout times:{}", ++count);
    if (count == 5) {
      ioContext.Stop();
    } else {
      timer.ExpiresAt(timer.GetExpiry() + std::chrono::milliseconds(500));
      timer.AsyncAwait(taskGenerator());
    }
    co_return;
  };
  timer.ExpiresAfter(std::chrono::milliseconds(500));
  timer.AsyncAwait(taskGenerator());
  ioContext.Start();
}

int main() {
  TestBasicSteadyTimerUsage();
  TestUseSteadyTimerRepeat();
}