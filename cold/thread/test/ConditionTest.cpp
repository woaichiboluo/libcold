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

void testTimeWait() {
  Base::LockGuard guard(g_mutex);
  auto before = Base::Time::now();
  g_condition.waitFor(std::chrono::milliseconds(300));
  auto now = Base::Time::now();
  assert(now - before >= std::chrono::milliseconds(300));
  auto t = now + std::chrono::milliseconds(500);
  g_condition.waitUntil(t);
  auto now1 = Base::Time::now();
  assert(now1 - now >= std::chrono::milliseconds(500));
}

void produce() {
  for (int i = 0; i < 1000; ++i) {
    Base::LockGuard guard(g_mutex);
    while (g_sequence.size() >= 20) {
      g_condition.wait();
    }
    g_sequence.push(++counter);
    std::cout << "produce " << counter << std::endl;
    g_condition.notifyOne();
  }
}

void consume() {
  for (int i = 0; i < 1000; ++i) {
    Base::LockGuard guard(g_mutex);
    while (g_sequence.empty()) {
      g_condition.wait();
    }
    auto value = g_sequence.front();
    g_sequence.pop();
    std::cout << "consume " << value << std::endl;
    g_condition.notifyOne();
  }
}

int main() {
  Base::Thread t1(produce);
  Base::Thread t2(consume);
  t1.start();
  t2.start();
  t1.join();
  t2.join();
  std::cout << "Begin test timewait" << std::endl;
  testTimeWait();
  std::cout << "Complete" << std::endl;
}