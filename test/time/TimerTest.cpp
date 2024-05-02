#include <chrono>

#include "cold/coro/IoService.h"
#include "cold/coro/Task.h"
#include "cold/log/Logger.h"
#include "cold/time/Timer.h"

using namespace Cold;

void TestBasicTimerUsage() {
  Base::INFO("In Testcase {}", __FUNCTION__);
  Base::IoService service;
  Base::Timer timer(service);
  auto expect = Base::Time::Now() + std::chrono::milliseconds(200);
  Base::INFO("Set Timer expect:{}", expect.Dump());
  timer.ExpiresAt(expect);
  timer.AsyncWait([](Base::IoService& s) -> Base::Task<> {
    s.Stop();
    Base::INFO("Timer timeout Stop the IoService");
    co_return;
  }(service));
  service.Start();
}

void TestUseTimerRepeat() {
  Base::INFO("In Testcase {}", __FUNCTION__);
  Base::IoService service;
  Base::Timer timer(service);
  int count = 0;
  std::function<Base::Task<>()> taskGenerator = [&]() -> Base::Task<> {
    Base::INFO("SteadyTimer Timeout times: {}", ++count);
    if (count == 5) {
      service.Stop();
    } else {
      auto expect = timer.GetExpiry() + std::chrono::milliseconds(500);
      Base::INFO("Reset SteadyTimer expect: {}", expect.Dump());
      timer.ExpiresAt(timer.GetExpiry() + std::chrono::milliseconds(500));
      timer.AsyncWait(taskGenerator());
    }
    co_return;
  };
  auto expect = Base::Time::Now() + std::chrono::milliseconds(500);
  Base::INFO("Set Timer per 500 ms triggle once repeat 5 times expect:{}",
             expect.Dump());
  timer.ExpiresAt(expect);
  timer.AsyncWait(taskGenerator());
  service.Start();
}

void TestUpdateExpiry() {
  Base::INFO("In Testcase {}", __FUNCTION__);
  Base::IoService service;
  Base::Timer timer(service);
  Base::Timer timer1(service);
  Base::Timer timer2(service);
  Base::Timer timer3(service);
  Base::INFO("0.Set Timer after 500 ms");
  Base::INFO("1.Set Timer1 after 1 sec");
  Base::INFO("2.Set Timer2 after 2 sec");
  Base::INFO("3.Set Timer3 after 3 sec");
  auto now = Base::Time::Now();
  timer.ExpiresAfter(std::chrono::milliseconds(500));
  timer1.ExpiresAfter(std::chrono::seconds(1));
  timer2.ExpiresAfter(std::chrono::seconds(2));
  timer3.ExpiresAfter(std::chrono::seconds(3));
  auto tFunc = [&]() -> Base::Task<void> {
    Base::INFO("0.Timer timeout");
    auto expect1 = now + std::chrono::seconds(3);
    Base::INFO("0.Update Timer1 TimeoutTime expect:{}", expect1.Dump());
    auto expect2 = now + std::chrono::seconds(2);
    Base::INFO("0.Update Timer2 TimeoutTime expect:{}", expect2.Dump());
    auto expect3 = now + std::chrono::seconds(1);
    Base::INFO("0.Update Timer3 TimeoutTime expect:{}", expect3.Dump());
    timer1.ExpiresAt(expect1);
    timer2.ExpiresAt(expect2);
    timer3.ExpiresAt(expect3);
    co_return;
  };
  timer.AsyncWait(tFunc());
  timer1.AsyncWait([](Base::IoService& s) -> Base::Task<> {
    Base::INFO("1.Timer1 timeout");
    s.Stop();
    co_return;
  }(service));
  timer2.AsyncWait([]() -> Base::Task<> {
    Base::INFO("2.Timer2 timeout");
    co_return;
  }());
  timer3.AsyncWait([]() -> Base::Task<> {
    Base::INFO("3.Timer3 timeout");
    co_return;
  }());
  service.Start();
}

void TestCancelTimer() {
  Base::IoService service;
  Base::Timer timer1(service), timer2(service), timer3(service),
      timer4(service);
  Base::INFO("In Testcase {}", __FUNCTION__);

  Base::INFO("1.Set Timer1 after 500 ms");
  Base::INFO("2.Set Timer2 after 1000 ms");
  Base::INFO("3.Set Timer3 after 1500 ms");
  Base::INFO("4.Set Timer4 after 2000 ms");

  timer1.ExpiresAfter(std::chrono::milliseconds(500));
  timer2.ExpiresAfter(std::chrono::milliseconds(1000));
  timer3.ExpiresAfter(std::chrono::milliseconds(1500));
  timer4.ExpiresAfter(std::chrono::milliseconds(2000));

  timer1.AsyncWait([]() -> Base::Task<> {
    Base::FATAL("This should not output");
    co_return;
  }());

  timer2.AsyncWait([](Base::Timer* timer) -> Base::Task<> {
    Base::INFO("2.Timer2 timeout cancel Timer3", __FUNCTION__);
    timer->Cancel();
    co_return;
  }(&timer3));

  timer3.AsyncWait([]() -> Base::Task<> {
    Base::FATAL("This should not output");
    co_return;
  }());

  timer4.AsyncWait([](Base::IoService& s) -> Base::Task<> {
    Base::INFO("4.Timer4 timeout Stop IoService");
    s.Stop();
    co_return;
  }(service));
  Base::INFO("Cancel Timer1");
  timer1.Cancel();
  service.Start();
}

void TestTimerAwaitable() {
  Base::INFO("In Testcase {}", __FUNCTION__);
  Base::IoService service;
  Base::Timer timer(service);
  auto coro = []() -> Base::Task<> {
    Base::INFO("return type: void");
    co_return;
  }();
  auto intCoro = []() -> Base::Task<int> {
    Base::INFO("Return type: int");
    co_return 100;
  }();
  auto rvalueCoro = []() -> Base::Task<std::unique_ptr<int>> {
    Base::INFO("Return type: std::unique_ptr<int>");
    co_return std::make_unique<int>(200);
  }();
  auto strCoro = []() -> Base::Task<std::string> {
    Base::INFO("Return type: std::string");
    co_return "Hello World";
  }();
  auto func = [&]() -> Base::Task<> {
    Base::INFO("1.co_await AsyncAwaitable void. wait 500ms");
    timer.ExpiresAfter(std::chrono::milliseconds(500));
    co_await timer.AsyncWaitable(std::move(coro));
    Base::INFO("1.AsyncAwaitable void complete.");

    Base::INFO("2.co_await AsyncAwaitable int. wait 500ms");
    timer.ExpiresAfter(std::chrono::milliseconds(500));
    auto v1 = co_await timer.AsyncWaitable(std::move(intCoro));
    Base::INFO("2.AsyncAwaitable int complete. value = {}", v1);

    Base::INFO("3.co_await AsyncAwaitable unique_ptr<int>. wait 500ms");
    timer.ExpiresAfter(std::chrono::milliseconds(500));
    auto v2 = co_await timer.AsyncWaitable(std::move(rvalueCoro));
    Base::INFO("3.AsyncAwaitable unique_ptr<int> complete. value = {}", *v2);

    Base::INFO("4.co_await AsyncAwaitable string. wait 500ms");
    timer.ExpiresAfter(std::chrono::milliseconds(500));
    auto v3 = co_await timer.AsyncWaitable(std::move(strCoro));
    Base::INFO("4.AsyncAwaitable string complete. value = {}", v3);

    service.Stop();
    co_return;
  };
  service.CoSpawn(func());
  service.Start();
}

void TestMoveTimer() {
  Base::INFO("In Testcase {}", __FUNCTION__);
  Base::IoService service;
  Base::Timer t1(service), t2(service);
  t1.ExpiresAfter(std::chrono::milliseconds(10));
  t1.AsyncWait([]() -> Base::Task<> {
    assert(false);
    co_return;
  }());
  t1 = std::move(t2);
  assert(t2.GetTimerId() == 0);
  auto task = [](Base::IoService& s, Base::Timer timer) -> Base::Task<> {
    timer.ExpiresAfter(std::chrono::seconds(1));
    co_await timer.AsyncWaitable([]() -> Base::Task<> {
      Base::INFO("After Move");
      co_return;
    }());
    s.Stop();
    co_return;
  }(service, std::move(t1));
  service.CoSpawn(std::move(task));
  assert(t1.GetTimerId() == 0);
  service.Start();
}

void TestTimerInMultiThread() {
  Base::INFO("In Testcase {}", __FUNCTION__);
  Base::IoService service;
  Base::Timer t1(service), t2(service);
  Base::Thread thread([&]() {
    Base::INFO("1.Set Timer1 after 1 sec");
    t1.ExpiresAfter(std::chrono::seconds(1));
    t1.AsyncWait([]() -> Base::Task<> {
      Base::INFO("1.Timer1 timeout");
      co_return;
    }());
  });
  thread.Start();
  Base::INFO("2.Set Timer2 after 2 sec");
  t2.ExpiresAfter(std::chrono::seconds(2));
  t2.AsyncWait([](Base::IoService& s) -> Base::Task<> {
    Base::INFO("2.Timer2 timeout");
    s.Stop();
    co_return;
  }(service));
  service.Start();
  thread.Join();
}

int main() {
  //   Base::LogManager::Instance().GetMainLoggerRaw()->SetLevel(
  //       Base::LogLevel::TRACE);
  Base::INFO("-------------------------------------");
  TestBasicTimerUsage();
  Base::INFO("-------------------------------------");
  TestUseTimerRepeat();
  Base::INFO("-------------------------------------");
  TestUpdateExpiry();
  Base::INFO("-------------------------------------");
  TestCancelTimer();
  Base::INFO("-------------------------------------");
  TestTimerAwaitable();
  Base::INFO("-------------------------------------");
  TestMoveTimer();
  Base::INFO("-------------------------------------");
  TestTimerInMultiThread();
}
