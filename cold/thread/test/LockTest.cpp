#include <iostream>
#include <thread>

#include "cold/thread/Lock.h"
#include "cold/thread/Thread.h"

using namespace Cold;

Base::Mutex g_mutex;
Base::SharedMutex g_sharedMutex;
int c GUARDED_BY(g_mutex) = 0;

void count() {
  for (int i = 0; i < 1000; ++i) {
    Base::LockGuard guard(g_mutex);
    ++c;
    std::cout << c << " " << std::this_thread::get_id() << std::endl;
  }
}

void count1() {
  for (int i = 0; i < 1000; ++i) {
    g_mutex.lock();
    Base::LockGuard guard(g_mutex, Base::AdoptLock);
    ++c;
    std::cout << c << " " << std::this_thread::get_id() << std::endl;
  }
}

void count2() {
  Base::LockGuard guard(g_mutex, Base::DeferLock);
  for (int i = 0; i < 1000; ++i) {
    guard.lock();
    ++c;
    std::cout << c << " " << std::this_thread::get_id() << std::endl;
    guard.unlock();
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
  t1.start();
  t2.start();
  t3.start();
  t4.start();
  t1.join();
  t2.join();
  t3.join();
  t4.join();
}

int main() {
  run(count);
  run(count1);
  run(count2);
  c = 0;
  Base::Thread t1(count4);
  t1.start();
  run(count3);
  t1.join();
}