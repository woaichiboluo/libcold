#ifndef COLD_COROUTINES_ASYNCMUTEX
#define COLD_COROUTINES_ASYNCMUTEX

#include <atomic>

#include "../detail/TaskBase.h"

// modify from cppcoro/async_mutex.hpp

namespace Cold {

class AsyncMutex {
 public:
  friend class AsyncMutexLockOperation;

  class AsyncMutexLockOperation {
   public:
    friend class AsyncMutex;
    explicit AsyncMutexLockOperation(AsyncMutex& mutex) noexcept
        : mutex_(mutex) {}
    virtual ~AsyncMutexLockOperation() = default;

    AsyncMutexLockOperation(const AsyncMutexLockOperation&) = delete;
    AsyncMutexLockOperation& operator=(const AsyncMutexLockOperation&) = delete;

    bool await_ready() const noexcept { return false; }

    bool await_suspend(std::coroutine_handle<> handle) noexcept {
      auto& promise = std::coroutine_handle<Detail::PromiseBase>::from_address(
                          handle.address())
                          .promise();
      coroContext_.handle = handle;
      coroContext_.exeucuteContext = promise.GetIoContext();
      coroContext_.token = promise.GetStopToken();
      auto head = mutex_.linkList_.load();
      while (true) {
        if (head == kNotLocked) {
          if (mutex_.linkList_.compare_exchange_strong(head,
                                                       kLockedNoWaiters)) {
            return false;
          }
        } else {
          next_ = reinterpret_cast<AsyncMutexLockOperation*>(head);
          if (mutex_.linkList_.compare_exchange_strong(
                  head, reinterpret_cast<uintptr_t>(this))) {
            return true;
          }
        }
      }
    }

    void await_resume() const noexcept {}

   private:
    struct CoroContext {
      IoContext* exeucuteContext = nullptr;
      std::coroutine_handle<> handle;
      std::stop_token token;
    };
    AsyncMutex& mutex_;
    AsyncMutexLockOperation* next_ = nullptr;
    CoroContext coroContext_;
  };

  class AsyncMutexGuard {
   public:
    friend class Condition;
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

  class AsyncMutexScopedLockOperation : public AsyncMutexLockOperation {
   public:
    explicit AsyncMutexScopedLockOperation(AsyncMutex& mutex) noexcept
        : AsyncMutexLockOperation(mutex) {}

    AsyncMutexGuard await_resume() noexcept { return AsyncMutexGuard(mutex_); }
  };

  AsyncMutex() noexcept = default;
  ~AsyncMutex() {
    assert(linkList_.load() == kNotLocked);
    assert(waitersHead_ == nullptr);
  }

  AsyncMutex(const AsyncMutex&) = delete;
  AsyncMutex& operator=(const AsyncMutex&) = delete;

  [[nodiscard]] AsyncMutexLockOperation Lock() {
    return AsyncMutexLockOperation(*this);
  }

  [[nodiscard]] AsyncMutexScopedLockOperation ScopedLock() {
    return AsyncMutexScopedLockOperation(*this);
  }

  bool Trylock() {
    auto expect = kNotLocked;
    return linkList_.compare_exchange_strong(expect, kLockedNoWaiters);
  }

  void Unlock() {
    auto head = linkList_.load();
    assert(head != kNotLocked);
    if (waitersHead_ == nullptr) {
      if (head == kLockedNoWaiters &&
          linkList_.compare_exchange_strong(head, kNotLocked)) {
        return;
      }
      head = linkList_.exchange(kLockedNoWaiters);
      auto cur = reinterpret_cast<AsyncMutexLockOperation*>(head);
      AsyncMutexLockOperation* newHead = nullptr;
      while (cur) {
        auto next = cur->next_;
        cur->next_ = newHead;
        newHead = cur;
        cur = next;
      }
      waitersHead_ = newHead;
    }
    assert(waitersHead_);
    auto* waiter = waitersHead_;
    waitersHead_ = waiter->next_;
    if (!waiter->coroContext_.token.stop_requested()) {
      waiter->coroContext_.handle.resume();
    }
  }

 private:
  static constexpr std::uintptr_t kNotLocked = 1;
  static constexpr std::uintptr_t kLockedNoWaiters = 0;

  std::atomic<std::uintptr_t> linkList_{kNotLocked};
  AsyncMutexLockOperation* waitersHead_ = nullptr;
};

}  // namespace Cold

#endif /* COLD_COROUTINES_ASYNCMUTEX */
