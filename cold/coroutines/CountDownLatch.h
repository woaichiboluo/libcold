#ifndef COLD_COROUTINES_COUNTDOWNLATCH
#define COLD_COROUTINES_COUNTDOWNLATCH

#include "Condition.h"

namespace Cold {

class CountDownLatch {
 public:
  CountDownLatch(int count) : count_(count) {}
  ~CountDownLatch() = default;

  CountDownLatch(const CountDownLatch&) = delete;
  CountDownLatch& operator=(const CountDownLatch&) = delete;

  [[nodiscard]] Task<> CountDown() {
    auto guard = co_await mutex_.ScopedLock();
    if (--count_ == 0) {
      condition_.NotifyAll();
    }
  }

  [[nodiscard]] Task<> Wait() {
    auto guard = co_await mutex_.ScopedLock();
    co_await condition_.Wait(guard, [this] { return count_ == 0; });
  }

 private:
  AsyncMutex mutex_;
  Condition condition_;
  int count_;
};

}  // namespace Cold

#endif /* COLD_COROUTINES_COUNTDOWNLATCH */
