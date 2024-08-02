#include "cold/coro/AsyncMutex.h"
#include "cold/coro/IoService.h"
#include "cold/log/Logger.h"
#include "cold/thread/Thread.h"

using namespace Cold;
Base::AsyncMutex g_mutex;

int g_count = 0;

Base::Task<> Add() {
  auto guard = co_await g_mutex.ScopedLock();
  g_count++;
}

int main() {
  Base::IoService s1, s2, s3, s4;
  std::vector<std::unique_ptr<Base::Thread>> threads;
  threads.emplace_back(std::make_unique<Base::Thread>([&s1]() {
    for (int i = 0; i < 100000; i++) {
      s1.CoSpawn(Add());
    }
    s1.Start();
  }));

  threads.emplace_back(std::make_unique<Base::Thread>([&s2]() {
    for (int i = 0; i < 100000; i++) {
      s2.CoSpawn(Add());
    }
    s2.Start();
  }));

  threads.emplace_back(std::make_unique<Base::Thread>([&s3]() {
    for (int i = 0; i < 100000; i++) {
      s3.CoSpawn(Add());
    }
    s3.Start();
  }));

  threads.emplace_back(std::make_unique<Base::Thread>([&s4]() {
    for (int i = 0; i < 100000; i++) {
      s4.CoSpawn(Add());
    }
    s4.Start();
  }));

  for (auto& t : threads) {
    t->Start();
  }

  while (g_count < 400000) {
  }

  Base::INFO("count: {}", g_count);

  s1.Stop();
  s2.Stop();
  s3.Stop();
  s4.Stop();

  for (auto& t : threads) {
    t->Join();
  }
  return 0;
}