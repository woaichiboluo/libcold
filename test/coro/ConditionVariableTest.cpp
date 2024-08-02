
#include <deque>
#include <thread>

#include "cold/coro/AsyncMutex.h"
#include "cold/coro/ConditionVariable.h"
#include "cold/coro/IoService.h"
#include "cold/log/Logger.h"

using namespace Cold;

Base::AsyncMutex g_mutex;
Base::ConditionVariable g_cv(g_mutex);

std::deque<std::string> g_messages;

Base::Task<> Produce() {
  for (int i = 0; i < 100; ++i) {
    auto guard = co_await g_mutex.ScopedLock();
    g_messages.push_back(fmt::format("Message-{}", i));
    g_cv.NotifyOne();
  }
}

Base::Task<> Consume(Base::IoService& pro, Base::IoService& con) {
  for (int i = 0; i < 100; ++i) {
    auto guard = co_await g_mutex.ScopedLock();
    co_await g_cv.Wait([&]() { return !g_messages.empty(); });
    auto message = g_messages.front();
    g_messages.pop_front();
    Base::INFO("Consume message: {}", message);
  }
  pro.Stop();
  con.Stop();
}

int main() {
  Base::IoService producer, consumer;
  std::thread t([&]() {
    producer.CoSpawn(Produce());
    producer.Start();
  });

  consumer.CoSpawn(Consume(producer, consumer));
  consumer.Start();

  t.join();
  return 0;
}