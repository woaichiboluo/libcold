#include <iostream>
#include <thread>

#include "cold/thread/Lock.h"
#include "cold/thread/Thread.h"

using namespace Cold;

Base::Mutex g_mutex;
Base::SharedMutex g_sharedMutex;
int b GUARDED_BY(g_mutex) = 0;
int c GUARDED_BY(g_sharedMutex) = 0;

void count() {
  for (int i = 0; i < 1000; ++i) {
    Base::LockGuard guard(g_mutex);
    ++b;
    std::cout << b << " " << std::this_thread::get_id() << std::endl;
  }
}

void count1() {
  for (int i = 0; i < 1000; ++i) {
    g_mutex.Lock();
    Base::LockGuard guard(g_mutex, Base::AdoptLock);
    ++b;
    std::cout << b << " " << std::this_thread::get_id() << std::endl;
  }
}

void count2() {
  Base::LockGuard guard(g_mutex, Base::DeferLock);
  for (int i = 0; i < 1000; ++i) {
    guard.Lock();
    ++b;
    std::cout << b << " " << std::this_thread::get_id() << std::endl;
    guard.Unlock();
  }
}

void count3() {
  for (int i = 0; i < 1000; ++i) {
    Base::SharedLockGuard guard(g_sharedMutex, Base::SharedLock);
    std::cout << "read:" << c << " " << std::this_thread::get_id() << std::endl;
  }
}

void count4() {
  for (int i = 0; i < 1000; ++i) {
    Base::SharedLockGuard guard(g_sharedMutex);
    ++c;
    std::cout << "write:" << c << " " << std::this_thread::get_id()
              << std::endl;
  }
}

template <typename Runnable>
void run(Runnable r) {
  Base::Thread t1(r);
  Base::Thread t2(r);
  Base::Thread t3(r);
  Base::Thread t4(r);
  t1.Start();
  t2.Start();
  t3.Start();
  t4.Start();
  t1.Join();
  t2.Join();
  t3.Join();
  t4.Join();
}

int main() {
  run(count);
  run(count1);
  run(count2);
  Base::Thread t1(count4);
  t1.Start();
  run(count3);
  t1.Join();
}