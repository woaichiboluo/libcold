#include <cassert>
#include <chrono>
#include <iostream>
#include <queue>

#include "cold/thread/Condition.h"
#include "cold/thread/Lock.h"
#include "cold/thread/Thread.h"
#include "cold/time/Time.h"

using namespace Cold;

Base::Mutex g_mutex;
Base::Condition g_condition(g_mutex);
std::queue<int> g_sequence GUARDED_BY(g_mutex);
int counter GUARDED_BY(g_mutex) = 0;

void TestTimeWait() {
  Base::LockGuard guard(g_mutex);
  auto before = Base::Time::Now();
  g_condition.WaitFor(std::chrono::milliseconds(300));
  auto now = Base::Time::Now();
  assert(now - before >= std::chrono::milliseconds(300));
  auto t = now + std::chrono::milliseconds(500);
  g_condition.WaitUntil(t);
  auto now1 = Base::Time::Now();
  assert(now1 - now >= std::chrono::milliseconds(500));
}

void Produce() {
  for (int i = 0; i < 1000; ++i) {
    Base::LockGuard guard(g_mutex);
    while (g_sequence.size() >= 20) {
      g_condition.Wait();
    }
    g_sequence.push(++counter);
    std::cout << "produce " << counter << std::endl;
    g_condition.NotifyOne();
  }
}

void Consume() {
  for (int i = 0; i < 1000; ++i) {
    Base::LockGuard guard(g_mutex);
    while (g_sequence.empty()) {
      g_condition.Wait();
    }
    auto value = g_sequence.front();
    g_sequence.pop();
    std::cout << "consume " << value << std::endl;
    g_condition.NotifyOne();
  }
}

int main() {
  Base::Thread t1(Produce);
  Base::Thread t2(Consume);
  t1.Start();
  t2.Start();
  t1.Join();
  t2.Join();
  std::cout << "Begin test timewait" << std::endl;
  TestTimeWait();
  std::cout << "Complete" << std::endl;
}