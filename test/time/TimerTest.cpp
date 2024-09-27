#include "cold/Cold.h"

using namespace Cold;

void TestBasicTimerUsage() {
  INFO("In Testcase {}", __FUNCTION__);
  IoContext context;
  Timer timer(context);
  auto expect = Time::Now() + std::chrono::milliseconds(200);
  INFO("Set Timer expect:{}", expect.Dump());
  timer.ExpiresAt(expect);
  timer.AsyncWait([](IoContext& c) -> Task<> {
    c.Stop();
    INFO("Timer timeout Stop the IoContext");
    co_return;
  }(context));
  context.Start();
}

void TestUseTimerRepeat() {
  INFO("In Testcase {}", __FUNCTION__);
  IoContext context;
  Timer timer(context);
  int count = 0;
  std::function<Task<>()> taskGenerator = [&]() -> Task<> {
    INFO("SteadyTimer Timeout times: {}", ++count);
    if (count == 5) {
      context.Stop();
    } else {
      auto expect = timer.GetExpiry() + std::chrono::milliseconds(500);
      INFO("Reset SteadyTimer expect: {}", expect.Dump());
      timer.ExpiresAt(timer.GetExpiry() + std::chrono::milliseconds(500));
      timer.AsyncWait(taskGenerator());
    }
    co_return;
  };
  auto expect = Time::Now() + std::chrono::milliseconds(500);
  INFO("Set Timer per 500 ms triggle once repeat 5 times expect:{}",
       expect.Dump());
  timer.ExpiresAt(expect);
  timer.AsyncWait(taskGenerator());
  context.Start();
}

void TestUpdateExpiry() {
  INFO("In Testcase {}", __FUNCTION__);
  IoContext context;
  Timer timer(context);
  Timer timer1(context);
  Timer timer2(context);
  Timer timer3(context);
  INFO("0.Set Timer after 500 ms");
  INFO("1.Set Timer1 after 1 sec");
  INFO("2.Set Timer2 after 2 sec");
  INFO("3.Set Timer3 after 3 sec");
  auto now = Time::Now();
  timer.ExpiresAfter(std::chrono::milliseconds(500));
  timer1.ExpiresAfter(std::chrono::seconds(1));
  timer2.ExpiresAfter(std::chrono::seconds(2));
  timer3.ExpiresAfter(std::chrono::seconds(3));
  auto tFunc = [&]() -> Task<void> {
    INFO("0.Timer timeout");
    auto expect1 = now + std::chrono::seconds(3);
    INFO("0.Update Timer1 TimeoutTime expect:{}", expect1.Dump());
    auto expect2 = now + std::chrono::seconds(2);
    INFO("0.Update Timer2 TimeoutTime expect:{}", expect2.Dump());
    auto expect3 = now + std::chrono::seconds(1);
    INFO("0.Update Timer3 TimeoutTime expect:{}", expect3.Dump());
    timer1.ExpiresAt(expect1);
    timer2.ExpiresAt(expect2);
    timer3.ExpiresAt(expect3);
    co_return;
  };
  timer.AsyncWait(tFunc());
  timer1.AsyncWait([](IoContext& s) -> Task<> {
    INFO("1.Timer1 timeout");
    s.Stop();
    co_return;
  }(context));
  timer2.AsyncWait([]() -> Task<> {
    INFO("2.Timer2 timeout");
    co_return;
  }());
  timer3.AsyncWait([]() -> Task<> {
    INFO("3.Timer3 timeout");
    co_return;
  }());
  context.Start();
}

void TestCancelTimer() {
  IoContext context;
  Timer timer1(context), timer2(context), timer3(context), timer4(context);
  INFO("In Testcase {}", __FUNCTION__);

  INFO("1.Set Timer1 after 500 ms");
  INFO("2.Set Timer2 after 1000 ms");
  INFO("3.Set Timer3 after 1500 ms");
  INFO("4.Set Timer4 after 2000 ms");

  timer1.ExpiresAfter(std::chrono::milliseconds(500));
  timer2.ExpiresAfter(std::chrono::milliseconds(1000));
  timer3.ExpiresAfter(std::chrono::milliseconds(1500));
  timer4.ExpiresAfter(std::chrono::milliseconds(2000));

  timer1.AsyncWait([]() -> Task<> {
    FATAL("This should not output");
    co_return;
  }());

  timer2.AsyncWait([](Timer* timer) -> Task<> {
    INFO("2.Timer2 timeout cancel Timer3", __FUNCTION__);
    timer->Cancel();
    co_return;
  }(&timer3));

  timer3.AsyncWait([]() -> Task<> {
    FATAL("This should not output");
    co_return;
  }());

  timer4.AsyncWait([](IoContext& s) -> Task<> {
    INFO("4.Timer4 timeout Stop IoContext");
    s.Stop();
    co_return;
  }(context));
  INFO("Cancel Timer1");
  timer1.Cancel();
  context.Start();
}

void TestTimerAwaitable() {
  INFO("In Testcase {}", __FUNCTION__);
  IoContext context;
  Timer timer(context);
  auto coro = []() -> Task<> {
    INFO("return type: void");
    co_return;
  }();
  auto intCoro = []() -> Task<int> {
    INFO("Return type: int");
    co_return 100;
  }();
  auto rvalueCoro = []() -> Task<std::unique_ptr<int>> {
    INFO("Return type: std::unique_ptr<int>");
    co_return std::make_unique<int>(200);
  }();
  auto strCoro = []() -> Task<std::string> {
    INFO("Return type: std::string");
    co_return "Hello World";
  }();
  auto func = [&]() -> Task<> {
    INFO("1.co_await AsyncAwaitable void. wait 500ms");
    timer.ExpiresAfter(std::chrono::milliseconds(500));
    co_await timer.AsyncWaitable(std::move(coro));
    INFO("1.AsyncAwaitable void complete.");

    INFO("2.co_await AsyncAwaitable int. wait 500ms");
    timer.ExpiresAfter(std::chrono::milliseconds(500));
    auto v1 = co_await timer.AsyncWaitable(std::move(intCoro));
    INFO("2.AsyncAwaitable int complete. value = {}", v1);

    INFO("3.co_await AsyncAwaitable unique_ptr<int>. wait 500ms");
    timer.ExpiresAfter(std::chrono::milliseconds(500));
    auto v2 = co_await timer.AsyncWaitable(std::move(rvalueCoro));
    INFO("3.AsyncAwaitable unique_ptr<int> complete. value = {}", *v2);

    INFO("4.co_await AsyncAwaitable string. wait 500ms");
    timer.ExpiresAfter(std::chrono::milliseconds(500));
    auto v3 = co_await timer.AsyncWaitable(std::move(strCoro));
    INFO("4.AsyncAwaitable string complete. value = {}", v3);

    context.Stop();
    co_return;
  };
  context.CoSpawn(func());
  context.Start();
}

void TestMoveTimer() {
  INFO("In Testcase {}", __FUNCTION__);
  IoContext context;
  Timer t1(context), t2(context);
  t1.ExpiresAfter(std::chrono::milliseconds(10));
  t1.AsyncWait([]() -> Task<> {
    assert(false);
    co_return;
  }());
  t1 = std::move(t2);
  assert(t2.GetTimerId() == 0);
  auto task = [](IoContext& s, Timer timer) -> Task<> {
    timer.ExpiresAfter(std::chrono::seconds(1));
    co_await timer.AsyncWaitable([]() -> Task<> {
      INFO("After Move");
      co_return;
    }());
    s.Stop();
    co_return;
  }(context, std::move(t1));
  context.CoSpawn(std::move(task));
  assert(t1.GetTimerId() == 0);
  context.Start();
}

void TestTimerInMultiThread() {
  INFO("In Testcase {}", __FUNCTION__);
  IoContext context;
  Timer t1(context), t2(context);
  Thread thread([&]() {
    INFO("1.Set Timer1 after 1 sec");
    t1.ExpiresAfter(std::chrono::seconds(1));
    t1.AsyncWait([]() -> Task<> {
      INFO("1.Timer1 timeout");
      co_return;
    }());
  });
  thread.Start();
  INFO("2.Set Timer2 after 2 sec");
  t2.ExpiresAfter(std::chrono::seconds(2));
  t2.AsyncWait([](IoContext& s) -> Task<> {
    INFO("2.Timer2 timeout");
    s.Stop();
    co_return;
  }(context));
  context.Start();
  thread.Join();
}

int main() {
  // LogManager::GetInstance().GetDefaultRaw()->SetLevel(LogLevel::TRACE);
  INFO("-------------------------------------");
  TestBasicTimerUsage();
  INFO("-------------------------------------");
  TestUseTimerRepeat();
  INFO("-------------------------------------");
  TestUpdateExpiry();
  INFO("-------------------------------------");
  TestCancelTimer();
  INFO("-------------------------------------");
  TestTimerAwaitable();
  INFO("-------------------------------------");
  TestMoveTimer();
  INFO("-------------------------------------");
  TestTimerInMultiThread();
}
