#include <iostream>
#include <memory>
#include <vector>

#include "cold/thread/Lock.h"
#include "cold/thread/Thread.h"

using namespace Cold;

Base::Mutex g_mutex;
Base::SharedMutex g_sharedMutex;
int b GUARDED_BY(g_mutex) = 0;
int c GUARDED_BY(g_sharedMutex) = 0;

void Count() {
  for (int i = 0; i < 1000; ++i) {
    Base::LockGuard guard(g_mutex);
    ++b;
    std::cout << b << " " << Base::ThisThread::ThreadId() << std::endl;
  }
}

void Count1() {
  for (int i = 0; i < 1000; ++i) {
    g_mutex.Lock();
    Base::LockGuard guard(g_mutex, Base::AdoptLock);
    ++b;
    std::cout << b << " " << Base::ThisThread::ThreadId() << std::endl;
  }
}

void Count2() {
  Base::LockGuard guard(g_mutex, Base::DeferLock);
  for (int i = 0; i < 1000; ++i) {
    guard.Lock();
    ++b;
    std::cout << b << " " << Base::ThisThread::ThreadId() << std::endl;
    guard.Unlock();
  }
}

void Count3() {
  for (int i = 0; i < 1000; ++i) {
    Base::SharedLockGuard guard(g_sharedMutex, Base::SharedLock);
    std::cout << "read:" << c << " " << Base::ThisThread::ThreadId()
              << std::endl;
  }
}

void Count4() {
  for (int i = 0; i < 1000; ++i) {
    Base::SharedLockGuard guard(g_sharedMutex);
    ++c;
    std::cout << "write:" << c << " " << Base::ThisThread::ThreadId()
              << std::endl;
  }
}

template <typename Callable>
void Run(Callable r) {
  std::vector<std::unique_ptr<Base::Thread>> threads;
  for (size_t i = 0; i < 4; ++i) {
    auto thread = std::make_unique<Base::Thread>(r);
    thread->Start();
    threads.push_back(std::move(thread));
  }
  for (auto& thread : threads) {
    thread->Join();
  }
}

int main() {
  Run(Count);
  Run(Count1);
  Run(Count2);
  Base::Thread t1(Count4);
  t1.Start();
  Run(Count3);
  t1.Join();
}