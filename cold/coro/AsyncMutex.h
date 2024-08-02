#ifndef COLD_CORO_ASYNCMUTEX
#define COLD_CORO_ASYNCMUTEX

#include <atomic>
#include <cassert>
#include <coroutine>

// modify from cppcoro/async_mutex.hpp

namespace Cold::Base {
class AsyncMutex {
 public:
  friend class AsyncMutexLockOperation;
  class AsyncMutexLockOperation {
   public:
    AsyncMutexLockOperation(AsyncMutex& mutex) noexcept : mutex_(mutex) {}
    ~AsyncMutexLockOperation() = default;

    bool await_ready() const noexcept { return false; }

    bool await_suspend(std::coroutine_handle<> awaiter) noexcept {
      awaiter_ = awaiter;
      auto oldState = mutex_.state_.load(std::memory_order_relaxed);
      while (true) {
        if (oldState == AsyncMutex::kNotLocked) {
          if (mutex_.state_.compare_exchange_weak(oldState, kLockedNoWaiters,
                                                  std::memory_order_acq_rel))
            return false;
        } else {
          //开始进行头插
          auto oldHead = reinterpret_cast<AsyncMutexLockOperation*>(oldState);
          next_ = oldHead;
          if (mutex_.state_.compare_exchange_weak(
                  oldState, reinterpret_cast<std::uintptr_t>(this),
                  std::memory_order_acq_rel))
            return true;
        }
      }
    }

    void await_resume() const noexcept {}

   protected:
    AsyncMutex& mutex_;

   private:
    friend class AsyncMutex;
    AsyncMutexLockOperation* next_{nullptr};
    std::coroutine_handle<> awaiter_{nullptr};
  };

  class AsyncMutexGuard {
   public:
    explicit AsyncMutexGuard(AsyncMutex& mutex) noexcept : mutex_(&mutex) {}
    ~AsyncMutexGuard() {
      if (mutex_) mutex_->Unlock();
    }

    AsyncMutexGuard(const AsyncMutexGuard&) = delete;
    AsyncMutexGuard& operator=(const AsyncMutexGuard&) = delete;

    AsyncMutexGuard(AsyncMutexGuard&& other) noexcept : mutex_(other.mutex_) {
      other.mutex_ = nullptr;
    }

   private:
    AsyncMutex* mutex_;
  };

  class AsyncMutexScpoedLockOperation : public AsyncMutexLockOperation {
   public:
    explicit AsyncMutexScpoedLockOperation(AsyncMutex& mutex) noexcept
        : AsyncMutexLockOperation(mutex) {}

    AsyncMutexGuard await_resume() noexcept { return AsyncMutexGuard(mutex_); }
  };

  AsyncMutex() noexcept = default;
  ~AsyncMutex() {
    assert(state_.load(std::memory_order_relaxed) == kNotLocked);
    assert(waitersHead_ == nullptr);
  }

  AsyncMutex(const AsyncMutex&) = delete;
  AsyncMutex& operator=(const AsyncMutex&) = delete;

  [[nodiscard]] AsyncMutexLockOperation Lock() {
    return AsyncMutexLockOperation(*this);
  }

  [[nodiscard]] AsyncMutexScpoedLockOperation ScopedLock() {
    return AsyncMutexScpoedLockOperation(*this);
  }

  bool TryLock() {
    auto expected = kNotLocked;
    return state_.compare_exchange_strong(expected, kLockedNoWaiters,
                                          std::memory_order_acquire,
                                          std::memory_order_relaxed);
  }

  void Unlock() {
    auto oldState = state_.load(std::memory_order_relaxed);
    assert(oldState != kNotLocked);
    if (waitersHead_ == nullptr) {
      if (oldState == kLockedNoWaiters &&
          state_.compare_exchange_strong(oldState, kNotLocked,
                                         std::memory_order_release,
                                         std::memory_order_relaxed)) {
        return;
      }
      // there is a linked list of waiters order: FILO
      // reverse it to FIFO
      AsyncMutexLockOperation* newHead = nullptr;
      oldState = state_.exchange(kLockedNoWaiters, std::memory_order_acquire);
      assert(oldState != kNotLocked && oldState != kLockedNoWaiters);
      auto cur = reinterpret_cast<AsyncMutexLockOperation*>(oldState);
      while (cur) {
        auto temp = cur->next_;
        cur->next_ = newHead;
        newHead = cur;
        cur = temp;
      }
      waitersHead_ = newHead;
    }
    assert(waitersHead_);
    auto head = waitersHead_;
    waitersHead_ = waitersHead_->next_;
    head->awaiter_.resume();
  }

 private:
  static constexpr std::uintptr_t kNotLocked = 1;
  static constexpr std::uintptr_t kLockedNoWaiters = 0;

  std::atomic<std::uintptr_t> state_{kNotLocked};
  AsyncMutexLockOperation* waitersHead_{nullptr};
};

}  // namespace Cold::Base

#endif