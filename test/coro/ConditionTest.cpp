#include <deque>

#include "cold/Cold.h"

using namespace Cold;

AsyncMutex g_mutex;
Condition g_condition;

AsyncMutex g_mutex2;
Condition g_condition2;

constexpr int g_produceTimes = 6 * 10000 / 2;

constexpr int g_cnsumeTimes = 10000;

std::atomic<int> g_count = 0;
bool g_stop = false;

std::deque<int> g_queue;

Task<> Produce() {
  for (int i = 0; i < g_produceTimes; ++i) {
    auto lock = co_await g_mutex.ScopedLock();
    // auto& context = co_await ThisCoro::GetIoContext();
    // co_await context.RunInThisContext();
    auto value = ++g_count;
    g_queue.push_back(value);
    INFO("produce value: {}", value);
    g_condition.NotifyAll();
  }
}

Task<> Consumer() {
  for (int i = 0; i < g_cnsumeTimes; ++i) {
    auto lock = co_await g_mutex.ScopedLock();
    co_await g_condition.Wait(lock, [&]() { return !g_queue.empty(); });
    // auto& context = co_await ThisCoro::GetIoContext();
    // co_await context.RunInThisContext();
    auto value = g_queue.front();
    g_queue.pop_front();
    INFO("consume value: {}", value);
    if (value == g_produceTimes * 2) {
      g_stop = true;
      g_condition2.NotifyOne();
    }
  }
}

Task<> Stop(IoContextPool& pool) {
  auto lock = co_await g_mutex2.ScopedLock();
  co_await g_condition2.Wait(lock, [&]() { return g_stop; });
  co_await pool.GetMainIoContext().RunInThisContext();
  pool.Stop();
}

int main() {
  IoContextPool pool(8);
  for (int i = 0; i < 2; ++i) {
    pool.GetNextIoContext().CoSpawn(Produce());
  }
  for (int i = 0; i < 6; ++i) {
    pool.GetNextIoContext().CoSpawn(Consumer());
  }
  pool.GetMainIoContext().CoSpawn(Stop(pool));
  pool.Start();
}