#ifndef COLD_CORO_CONDITIONVARIABLE
#define COLD_CORO_CONDITIONVARIABLE

#include <atomic>
#include <cassert>
#include <coroutine>

#include "cold/coro/AsyncMutex.h"
#include "cold/coro/Task.h"

namespace Cold::Base {

class ConditionVariable {
  friend class ConditionVariableAwaiter;

 public:
  class ConditionVariableAwaiter {
   public:
    ConditionVariableAwaiter(ConditionVariable& cv) : cv_(cv) {}
    ~ConditionVariableAwaiter() = default;

    bool await_ready() const noexcept { return false; }

    void await_suspend(std::coroutine_handle<> awaiter) noexcept {
      awaiter_ = awaiter;
      next_ = cv_.head_.load(std::memory_order_relaxed);
      while (!cv_.head_.compare_exchange_weak(next_, this,
                                              std::memory_order_acq_rel))
        ;
      cv_.mutex_->Unlock();
    }

    void await_resume() const noexcept {}

   private:
    friend class ConditionVariable;
    ConditionVariable& cv_;
    ConditionVariableAwaiter* next_{nullptr};
    std::coroutine_handle<> awaiter_;
  };

  ConditionVariable(AsyncMutex& mutex) : mutex_(&mutex) {}
  ~ConditionVariable() = default;

  ConditionVariable(ConditionVariable&) = delete;
  ConditionVariable& operator=(ConditionVariable&) = delete;

  void NotifyOne() {
    ConditionVariableAwaiter* cur = nullptr;
    do {
      cur = head_.load(std::memory_order_relaxed);
    } while (cur != nullptr && !head_.compare_exchange_weak(
                                   cur, cur->next_, std::memory_order_acq_rel));
    if (cur == nullptr) return;
    cur->awaiter_.resume();
  }

  void NotifyAll() {
    ConditionVariableAwaiter* cur = nullptr;
    do {
      cur = head_.load(std::memory_order_relaxed);
    } while (cur != nullptr && !head_.compare_exchange_weak(
                                   cur, nullptr, std::memory_order_acq_rel));
    if (cur == nullptr) return;
    for (auto i = cur; i != nullptr; i = i->next_) {
      i->awaiter_.resume();
    }
  }

  template <typename Predicate>
  Base::Task<> Wait(Predicate cond) {
    while (!cond()) {
      co_await ConditionVariableAwaiter(*this);
      co_await mutex_->Lock();
    }
  }

 private:
  AsyncMutex* mutex_;
  std::atomic<ConditionVariableAwaiter*> head_{nullptr};
};

}  // namespace Cold::Base

#endif /* COLD_CORO_CONDITIONVARIABLE */
