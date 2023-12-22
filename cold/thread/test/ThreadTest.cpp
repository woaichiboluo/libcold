#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "cold/thread/Thread.h"
#include "cold/util/StringUtil.h"
#include "third_party/doctest.h"

using namespace Cold;

TEST_CASE("test main thread") {
  CHECK(Base::ThisThread::threadName() == "main");
  CHECK(Base::ThisThread::threadId() != 0);
  CHECK(Base::ThisThread::threadIdStr() ==
        Base::intToStr(Base::ThisThread::threadId()));
}

TEST_CASE("test detach") {
  std::string seq = "hello world";
  Base::Thread thread([=]() {
    std::string t;
    for (size_t i = 0; i < 1000; ++i) {
      t += seq;
    }
  });
  CHECK(thread.getThreadName() == "Thread1");
  CHECK(thread.getThreadId() == 0);
  thread.start();
  CHECK(thread.getThreadName() == "Thread1");
  CHECK(thread.getThreadId() != 0);
}

TEST_CASE("test multi thread") {
  Base::Thread t1([=]() {
    CHECK(Base::ThisThread::threadName() == "Thread2");
    CHECK(Base::ThisThread::threadId() != 0);
    CHECK(Base::ThisThread::threadIdStr() ==
          Base::intToStr(Base::ThisThread::threadId()));
  });
  Base::Thread t2([=]() {
    CHECK(Base::ThisThread::threadName() == "Thread3");
    CHECK(Base::ThisThread::threadId() != 0);
    CHECK(Base::ThisThread::threadIdStr() ==
          Base::intToStr(Base::ThisThread::threadId()));
  });
  Base::Thread t3(
      [=]() {
        CHECK(Base::ThisThread::threadName() == "Custom");
        CHECK(Base::ThisThread::threadId() != 0);
        CHECK(Base::ThisThread::threadIdStr() ==
              Base::intToStr(Base::ThisThread::threadId()));
      },
      "Custom");
  CHECK(t1.getThreadId() == 0);
  CHECK(t2.getThreadId() == 0);
  CHECK(t3.getThreadId() == 0);
  t1.start();
  t2.start();
  t3.start();
  CHECK(t1.getThreadId() != 0);
  CHECK(t2.getThreadId() != 0);
  CHECK(t3.getThreadId() != 0);
  CHECK(t1.joinable());
  CHECK(t2.joinable());
  CHECK(t3.joinable());
  t1.join();
  t2.join();
  t3.join();
}