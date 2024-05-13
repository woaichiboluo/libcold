#ifndef COLD_CORO_ASYNCMUTEX
#define COLD_CORO_ASYNCMUTEX

#include <atomic>
#include <cassert>
#include <coroutine>
#include <mutex>

// modify from cppcoro/async_mutex.hpp

namespace Cold::Base {

class AsyncMutex {
 public:
  class AsyncMutexLockOperation {
    friend class AsyncMutex;

   public:
    explicit AsyncMutexLockOperation(AsyncMutex& mutex) noexcept
        : mutex_(mutex) {}

    bool await_ready() const noexcept { return false; }
    bool await_suspend(std::coroutine_handle<> awaiter) noexcept {
      awaiter_ = awaiter;

      std::uintptr_t oldState = mutex_.state_.load(std::memory_order_acquire);
      while (true) {
        if (oldState == AsyncMutex::kNotLocked) {
          if (mutex_.state_.compare_exchange_weak(
                  oldState, AsyncMutex::kLockedNoWaiters,
                  std::memory_order_acquire, std::memory_order_relaxed)) {
            return false;
          }
        } else {
          next_ = reinterpret_cast<AsyncMutexLockOperation*>(oldState);
          if (mutex_.state_.compare_exchange_weak(
                  oldState, reinterpret_cast<std::uintptr_t>(this),
                  std::memory_order_release, std::memory_order_relaxed)) {
            // Queued operation to waiters list, suspend now.
            return true;
          }
        }
      }
    }
    void await_resume() const noexcept {}

   private:
    AsyncMutex& mutex_;
    AsyncMutexLockOperation* next_;
    std::coroutine_handle<> awaiter_;
  };

  AsyncMutex() noexcept : state_(kNotLocked), waiters_(nullptr) {}

  ~AsyncMutex() {
    [[maybe_unused]] auto state = state_.load(std::memory_order_relaxed);
    assert(state == kNotLocked || state == kLockedNoWaiters);
    assert(waiters_ == nullptr);
  }

  AsyncMutex(const AsyncMutex&) = delete;
  AsyncMutex& operator=(const AsyncMutex&) = delete;

  bool TryLock() noexcept {
    auto oldState = kNotLocked;
    return state_.compare_exchange_strong(oldState, kLockedNoWaiters,
                                          std::memory_order_acquire,
                                          std::memory_order_relaxed);
  }

  AsyncMutexLockOperation LockAsync() noexcept {
    return AsyncMutexLockOperation{*this};
  }

  class AsyncMutexLock {
   public:
    explicit AsyncMutexLock(AsyncMutex& mutex, std::adopt_lock_t) noexcept
        : mutex_(&mutex) {}

    AsyncMutexLock(AsyncMutexLock&& other) noexcept : mutex_(other.mutex_) {
      other.mutex_ = nullptr;
    }

    AsyncMutexLock(const AsyncMutexLock& other) = delete;
    AsyncMutexLock& operator=(const AsyncMutexLock& other) = delete;

    ~AsyncMutexLock() {
      if (mutex_ != nullptr) {
        mutex_->Unlock();
      }
    }

   private:
    AsyncMutex* mutex_;
  };

  class AsyncMutexScopedLockOperation : public AsyncMutexLockOperation {
   public:
    using AsyncMutexLockOperation::AsyncMutexLockOperation;

    [[nodiscard]] AsyncMutexLock await_resume() const noexcept {
      return AsyncMutexLock(mutex_, std::adopt_lock);
    }
  };

  AsyncMutexScopedLockOperation ScopedLockAsync() noexcept {
    return AsyncMutexScopedLockOperation(*this);
  }

  void Unlock() {
    assert(state_.load(std::memory_order_relaxed) != kNotLocked);
    AsyncMutexLockOperation* waitersHead = waiters_;
    if (waitersHead == nullptr) {
      auto oldState = kLockedNoWaiters;
      const bool releasedLock = state_.compare_exchange_strong(
          oldState, kNotLocked, std::memory_order_release,
          std::memory_order_relaxed);
      if (releasedLock) {
        return;
      }

      oldState = state_.exchange(kLockedNoWaiters, std::memory_order_acquire);
      assert(oldState != kLockedNoWaiters && oldState != kNotLocked);

      auto* next = reinterpret_cast<AsyncMutexLockOperation*>(oldState);
      do {
        auto* temp = next->next_;
        next->next_ = waitersHead;
        waitersHead = next;
        next = temp;
      } while (next != nullptr);
    }

    assert(waitersHead != nullptr);

    waiters_ = waitersHead->next_;

    waitersHead->awaiter_.resume();
  }

 private:
  static constexpr std::uintptr_t kNotLocked = 1;

  static constexpr std::uintptr_t kLockedNoWaiters = 0;

  std::atomic<std::uintptr_t> state_;

  AsyncMutexLockOperation* waiters_;
};

}  // namespace Cold::Base

#endif