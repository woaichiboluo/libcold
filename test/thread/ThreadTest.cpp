#include <string>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "cold/thread/Thread.h"
#include "third_party/doctest.h"

using namespace Cold;

TEST_CASE("test main thread") {
  CHECK(Base::ThisThread::ThreadName() == "main");
  CHECK(Base::ThisThread::ThreadId() != 0);
  CHECK(Base::ThisThread::ThreadIdStr() ==
        std::to_string(Base::ThisThread::ThreadId()));
}

TEST_CASE("test detach") {
  std::string seq = "hello world";
  Base::Thread thread([=]() {
    std::string t;
    for (size_t i = 0; i < 1000; ++i) {
      t += seq;
    }
  });
  CHECK(thread.GetThreadName() == "Thread-1");
  CHECK(thread.GetThreadId() == 0);
  thread.Start();
  CHECK(thread.GetThreadName() == "Thread-1");
  CHECK(thread.GetThreadId() != 0);
}

TEST_CASE("test multi thread") {
  Base::Thread t1([=]() {
    CHECK(Base::ThisThread::ThreadName() == "Thread-2");
    CHECK(Base::ThisThread::ThreadId() != 0);
    CHECK(Base::ThisThread::ThreadIdStr() ==
          std::to_string(Base::ThisThread::ThreadId()));
  });
  Base::Thread t2([=]() {
    CHECK(Base::ThisThread::ThreadName() == "Thread-3");
    CHECK(Base::ThisThread::ThreadId() != 0);
    CHECK(Base::ThisThread::ThreadIdStr() ==
          std::to_string(Base::ThisThread::ThreadId()));
  });
  Base::Thread t3(
      [=]() {
        CHECK(Base::ThisThread::ThreadName() == "Custom");
        CHECK(Base::ThisThread::ThreadId() != 0);
        CHECK(Base::ThisThread::ThreadIdStr() ==
              std::to_string(Base::ThisThread::ThreadId()));
      },
      "Custom");
  CHECK(t1.GetThreadId() == 0);
  CHECK(t2.GetThreadId() == 0);
  CHECK(t3.GetThreadId() == 0);
  t1.Start();
  t2.Start();
  t3.Start();
  CHECK(t1.GetThreadId() != 0);
  CHECK(t2.GetThreadId() != 0);
  CHECK(t3.GetThreadId() != 0);
  CHECK(t1.Joinable());
  CHECK(t2.Joinable());
  CHECK(t3.Joinable());
  t1.Join();
  t2.Join();
  t3.Join();
}