#include "cold/Cold.h"

using namespace Cold;
AsyncMutex g_mutex;

constexpr int g_times = 100000;
int g_count = 0;

Task<> Add() {
  for (int i = 0; i < g_times; ++i) {
    auto guard = co_await g_mutex.ScopedLock();
    g_count++;
  }
}

int main() {
  // LogManager::GetInstance().GetDefaultRaw()->SetLevel(LogLevel::TRACE);
  IoContext s1, s2, s3, s4;
  std::vector<std::unique_ptr<Thread>> threads;
  threads.emplace_back(std::make_unique<Thread>([&s1]() {
    for (int i = 0; i < 1; i++) {
      s1.CoSpawn(Add());
    }
    s1.Start();
  }));

  threads.emplace_back(std::make_unique<Thread>([&s2]() {
    for (int i = 0; i < 1; i++) {
      s2.CoSpawn(Add());
    }
    s2.Start();
  }));

  threads.emplace_back(std::make_unique<Thread>([&s3]() {
    for (int i = 0; i < 1; i++) {
      s3.CoSpawn(Add());
    }
    s3.Start();
  }));

  threads.emplace_back(std::make_unique<Thread>([&s4]() {
    for (int i = 0; i < 1; i++) {
      s4.CoSpawn(Add());
    }
    s4.Start();
  }));

  for (auto& t : threads) {
    t->Start();
  }

  while (g_count < 4 * g_times) {
  }

  INFO("count: {}", g_count);

  s1.Stop();
  s2.Stop();
  s3.Stop();
  s4.Stop();

  for (auto& t : threads) {
    t->Join();
  }
  return 0;
}