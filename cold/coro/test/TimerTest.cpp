#include <chrono>
#include <ratio>
#include <thread>

#include "cold/coro/IoContext.h"
#include "cold/coro/Task.h"
#include "cold/coro/Timer.h"
#include "cold/log/LogCommon.h"
#include "cold/log/Logger.h"
#include "cold/thread/Thread.h"

using namespace Cold::Base;

namespace {
auto g_logger = GetMainLogger();
}

void TestBasicSteadyTimerUsage() {
  LOG_INFO(g_logger, "In Testcase {}", __FUNCTION__);
  IoContext ioContext;
  SteadyTimer timer(ioContext);
  auto expect = Time::Now() + std::chrono::milliseconds(200);
  LOG_INFO(g_logger, "Set Timer expect:{}", expect.Dump());
  timer.ExpiresAt(expect);
  timer.AsyncWait([](IoContext& context) -> Task<void> {
    context.Stop();
    LOG_INFO(g_logger, "Timer timeout Stop the IoContext");
    co_return;
  }(ioContext));
  ioContext.Start();
};

void TestUseSteadyTimerRepeat() {
  LOG_INFO(g_logger, "In Testcase {}", __FUNCTION__);
  IoContext ioContext;
  SteadyTimer timer(ioContext);
  int count = 0;
  std::function<Task<>()> taskGenerator = [&]() -> Task<> {
    LOG_INFO(g_logger, "SteadyTimer Timeout times:{}", ++count);
    if (count == 5) {
      ioContext.Stop();
    } else {
      auto expect = timer.GetExpiry() + std::chrono::milliseconds(500);
      LOG_INFO(g_logger, "Reset SteadyTimer expect:{}", expect.Dump());
      timer.ExpiresAt(timer.GetExpiry() + std::chrono::milliseconds(500));
      timer.AsyncWait(taskGenerator());
    }
    co_return;
  };
  auto expect = Time::Now() + std::chrono::milliseconds(500);
  LOG_INFO(g_logger,
           "Set Timer per 500 ms triggle once repeat 5 times expect:{}",
           expect.Dump());
  timer.ExpiresAt(expect);
  timer.AsyncWait(taskGenerator());
  ioContext.Start();
}

void TestUpdateExpiry() {
  LOG_INFO(g_logger, "In Testcase {}", __FUNCTION__);
  IoContext ioContext;
  SteadyTimer timer(ioContext);
  SteadyTimer timer1(ioContext);
  SteadyTimer timer2(ioContext);
  SteadyTimer timer3(ioContext);
  LOG_INFO(g_logger, "0.Set Timer after 500 ms");
  LOG_INFO(g_logger, "1.Set Timer1 after 1 sec");
  LOG_INFO(g_logger, "2.Set Timer2 after 2 sec");
  LOG_INFO(g_logger, "3.Set Timer3 after 3 sec");
  auto now = Time::Now();
  timer.ExpiresAfter(std::chrono::milliseconds(500));
  timer1.ExpiresAfter(std::chrono::seconds(1));
  timer2.ExpiresAfter(std::chrono::seconds(2));
  timer3.ExpiresAfter(std::chrono::seconds(3));
  auto tFunc = [&]() -> Task<void> {
    LOG_INFO(g_logger, "0.Timer timeout");
    auto expect1 = now + std::chrono::seconds(3);
    LOG_INFO(g_logger, "0.Update Timer1 TimeoutTime expect:{}", expect1.Dump());
    auto expect2 = now + std::chrono::seconds(2);
    LOG_INFO(g_logger, "0.Update Timer2 TimeoutTime expect:{}", expect2.Dump());
    auto expect3 = now + std::chrono::seconds(1);
    LOG_INFO(g_logger, "0.Update Timer3 TimeoutTime expect:{}", expect3.Dump());
    timer1.ExpiresAt(expect1);
    timer2.ExpiresAt(expect2);
    timer3.ExpiresAt(expect3);
    co_return;
  };
  timer.AsyncWait(tFunc());
  timer1.AsyncWait([](IoContext& context) -> Task<void> {
    LOG_INFO(g_logger, "1.Timer1 timeout");
    context.Stop();
    co_return;
  }(ioContext));
  timer2.AsyncWait([]() -> Task<void> {
    LOG_INFO(g_logger, "2.Timer2 timeout");
    co_return;
  }());
  timer3.AsyncWait([]() -> Task<void> {
    LOG_INFO(g_logger, "3.Timer3 timeout");
    co_return;
  }());
  ioContext.Start();
}

void TestCancelTimer() {
  IoContext ioContext;
  SteadyTimer timer1(ioContext), timer2(ioContext), timer3(ioContext),
      timer4(ioContext);
  LOG_INFO(g_logger, "In Testcase {}", __FUNCTION__);

  LOG_INFO(g_logger, "1.Set Timer1 after 500 ms");
  LOG_INFO(g_logger, "2.Set Timer2 after 1000 ms");
  LOG_INFO(g_logger, "3.Set Timer3 after 1500 ms");
  LOG_INFO(g_logger, "4.Set Timer4 after 2000 ms");

  timer1.ExpiresAfter(std::chrono::milliseconds(500));
  timer2.ExpiresAfter(std::chrono::milliseconds(1000));
  timer3.ExpiresAfter(std::chrono::milliseconds(1500));
  timer4.ExpiresAfter(std::chrono::milliseconds(2000));

  timer1.AsyncWait([]() -> Task<void> {
    LOG_FATAL(g_logger, "This should not output");
    co_return;
  }());

  timer2.AsyncWait([](Timer* timer) -> Task<void> {
    LOG_INFO(g_logger, "2.Timer2 timeout cancel Timer3", __FUNCTION__);
    timer->Cancel();
    co_return;
  }(&timer3));

  timer3.AsyncWait([]() -> Task<void> {
    LOG_FATAL(g_logger, "This should not output");
    co_return;
  }());

  timer4.AsyncWait([](IoContext& context) -> Task<void> {
    LOG_INFO(g_logger, "4.Timer4 timeout Stop ioContext");
    context.Stop();
    co_return;
  }(ioContext));
  LOG_INFO(g_logger, "Cancel Timer1");
  timer1.Cancel();
  ioContext.Start();
}

void TestSteadyTimerAwaitable() {
  LOG_INFO(g_logger, "In Testcase {}", __FUNCTION__);
  IoContext ioContext;
  SteadyTimer timer(ioContext);
  auto coro = []() -> Task<void> {
    LOG_INFO(g_logger, "Do nothing coro");
    co_return;
  }();
  auto intCoro = []() -> Task<int> {
    LOG_INFO(g_logger, "Return int coro");
    co_return 100;
  }();
  auto rvalueCoro = []() -> Task<std::unique_ptr<int>> {
    LOG_INFO(g_logger, "Return int coro");
    co_return std::make_unique<int>(200);
  }();
  auto strCoro = []() -> Task<std::string> {
    LOG_INFO(g_logger, "Return int coro");
    co_return "Hello World";
  }();
  auto func = [&]() -> Task<void> {
    LOG_INFO(g_logger, "1.co_await AsyncAwaitable void. wait 500ms");
    timer.ExpiresAfter(std::chrono::milliseconds(500));
    co_await timer.AsyncWaitable(std::move(coro));
    LOG_INFO(g_logger, "1.AsyncAwaitable void complete.");

    LOG_INFO(g_logger, "2.co_await AsyncAwaitable int. wait 500ms");
    timer.ExpiresAfter(std::chrono::milliseconds(500));
    auto v1 = co_await timer.AsyncWaitable(std::move(intCoro));
    LOG_INFO(g_logger, "2.AsyncAwaitable int complete. value = {}", v1);

    LOG_INFO(g_logger, "3.co_await AsyncAwaitable unique_ptr<int>. wait 500ms");
    timer.ExpiresAfter(std::chrono::milliseconds(500));
    auto v2 = co_await timer.AsyncWaitable(std::move(rvalueCoro));
    LOG_INFO(g_logger, "3.AsyncAwaitable unique_ptr<int> complete. value = {}",
             *v2);

    LOG_INFO(g_logger, "4.co_await AsyncAwaitable string. wait 500ms");
    timer.ExpiresAfter(std::chrono::milliseconds(500));
    auto v3 = co_await timer.AsyncWaitable(std::move(strCoro));
    LOG_INFO(g_logger, "4.AsyncAwaitable string complete. value = {}", v3);

    ioContext.Stop();
    co_return;
  };
  ioContext.CoSpawn(func());
  ioContext.Start();
}

void TestSleep() {
  LOG_INFO(g_logger, "In Testcase {}", __FUNCTION__);
  IoContext ioContext;

  auto coro = [](IoContext& context) -> Task<void> {
    LOG_INFO(g_logger, "0.In coro Sleep 2 sec");
    co_await Sleep(context, std::chrono::seconds(2));
    LOG_INFO(g_logger, "0.in coro After Sleep");
  }(ioContext);

  RepeatedTimer timer(ioContext);

  int count = 0;

  auto taskGenerator = [&]() -> Task<void> {
    LOG_INFO(g_logger, "1.RepeateTimer times:{}", ++count);
    if (count == 6) {
      ioContext.Stop();
    }
    co_return;
  };

  LOG_INFO(g_logger, "1.Set RepeateTimer interval 500ms");
  timer.SetInterval(std::chrono::milliseconds(500));
  timer.AsyncWait(std::move(taskGenerator));
  ioContext.CoSpawn(std::move(coro));
  ioContext.Start();
}

void TestRepeatedTimer() {
  LOG_INFO(g_logger, "In Testcase {}", __FUNCTION__);
  IoContext context;
  RepeatedTimer timer1(context);
  RepeatedTimer timer2(context);
  int t1Counter = 0;
  int t2Counter = 0;

  auto t1 = [&]() -> Task<void> {
    LOG_INFO(g_logger, "1.RepeatedTimer1 Timeout times:{}", ++t1Counter);
    if (t1Counter == 10) {
      LOG_INFO(g_logger, "1.Cancel RepeatedTimer1");
      timer1.Cancel();
    } else if (t1Counter == 5) {
      LOG_INFO(g_logger, "1.Reset RepeatedTimer1 interval 300ms");
      timer1.SetInterval(std::chrono::milliseconds(300));
    }
    co_return;
  };

  auto t2 = [&]() -> Task<void> {
    LOG_INFO(g_logger, "2.RepeatedTimer2 Timeout times:{}", ++t2Counter);
    if (t2Counter == 10) {
      LOG_INFO(g_logger, "2.RepeatedTimer2 Stop IoContext");
      context.Stop();
    }
    co_return;
  };

  LOG_INFO(g_logger, "1.Set RepeatedTimer1 Interval 100ms");
  LOG_INFO(g_logger, "2.Set RepeatedTimer2 Interval 300ms");
  timer1.SetInterval(std::chrono::milliseconds(100));
  timer2.SetInterval(std::chrono::milliseconds(300));
  timer1.AsyncWait(std::move(t1));
  timer2.AsyncWait(std::move(t2));
  context.Start();
}

void TestTimerInMultiThread() {
  LOG_INFO(g_logger, "In Testcase {}", __FUNCTION__);
  IoContext ioContext;
  SteadyTimer t1(ioContext), t2(ioContext);
  Thread thread([&]() {
    LOG_INFO(g_logger, "1.Set Timer1 after 1 sec");
    t1.ExpiresAfter(std::chrono::seconds(1));
    t1.AsyncWait([]() -> Task<void> {
      LOG_INFO(g_logger, "1.Timer1 timeout");
      co_return;
    }());
  });
  thread.Start();
  LOG_INFO(g_logger, "2.Set Timer2 after 2 sec");
  t2.ExpiresAfter(std::chrono::seconds(2));
  t2.AsyncWait([](IoContext& context) -> Task<void> {
    LOG_INFO(g_logger, "2.Timer2 timeout");
    context.Stop();
    co_return;
  }(ioContext));
  ioContext.Start();
  thread.Join();
}

int main() {
  LOG_INFO(g_logger, "-------------------------------------");
  TestBasicSteadyTimerUsage();
  LOG_INFO(g_logger, "-------------------------------------");
  TestUseSteadyTimerRepeat();
  LOG_INFO(g_logger, "-------------------------------------");
  TestUpdateExpiry();
  LOG_INFO(g_logger, "-------------------------------------");
  TestCancelTimer();
  LOG_INFO(g_logger, "-------------------------------------");
  TestSteadyTimerAwaitable();
  LOG_INFO(g_logger, "-------------------------------------");
  TestSleep();
  LOG_INFO(g_logger, "-------------------------------------");
  TestRepeatedTimer();
  LOG_INFO(g_logger, "-------------------------------------");
  TestTimerInMultiThread();
  LOG_INFO(g_logger, "-------------------------------------");
}