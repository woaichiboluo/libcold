#ifndef COLD_COROUTINES_CONDITION
#define COLD_COROUTINES_CONDITION

#include "AsyncMutex.h"
#include "Task.h"

namespace Cold {

class Condition {
 public:
  friend class ConditionWaitOperation;

  Condition() = default;
  ~Condition() = default;

  Condition(const Condition&) = delete;
  Condition& operator=(const Condition&) = delete;

  class ConditionWaitOperation {
   public:
    friend class Condition;
    ConditionWaitOperation(Condition& condition, AsyncMutex& mutex) noexcept
        : condition_(condition), mutex_(mutex) {}

    ~ConditionWaitOperation() = default;

    bool await_ready() const noexcept { return false; }

    void await_suspend(std::coroutine_handle<> handle) noexcept {
      auto& promise = std::coroutine_handle<Detail::PromiseBase>::from_address(
                          handle.address())
                          .promise();
      coroContext_.handle = handle;
      coroContext_.token = promise.GetStopToken();
      auto head = condition_.linkList_.load();
      while (true) {
        next_ = reinterpret_cast<ConditionWaitOperation*>(head);
        if (condition_.linkList_.compare_exchange_strong(
                head, reinterpret_cast<uintptr_t>(this))) {
          break;
        }
      }
      mutex_.Unlock();
    }

    void await_resume() const noexcept {}

   private:
    struct CoroContext {
      std::coroutine_handle<> handle;
      std::stop_token token;
    };
    Condition& condition_;
    AsyncMutex& mutex_;
    CoroContext coroContext_;
    ConditionWaitOperation* next_ = nullptr;
  };

  [[nodiscard]] Task<> Wait(AsyncMutex::AsyncMutexGuard& lock) {
    co_await ConditionWaitOperation(*this, *lock.mutex_);
    co_await lock.mutex_->Lock();
  }

  template <typename Predicate>
  [[nodiscard]] Task<> Wait(AsyncMutex::AsyncMutexGuard& lock, Predicate cond) {
    while (!cond()) {
      co_await ConditionWaitOperation(*this, *lock.mutex_);
      co_await lock.mutex_->Lock();
    }
  }

  void NotifyOne() {
    Notify();
    if (waitersHead_ == nullptr) return;
    auto* waiter = waitersHead_;
    waitersHead_ = waiter->next_;
    if (!waiter->coroContext_.token.stop_requested()) {
      waiter->coroContext_.handle.resume();
    }
  }

  void NotifyAll() {
    Notify();
    if (waitersHead_ == nullptr) return;
    auto* cur = waitersHead_;
    waitersHead_ = nullptr;
    while (cur) {
      if (!cur->coroContext_.token.stop_requested()) {
        cur->coroContext_.handle.resume();
      }
      cur = cur->next_;
    }
  }

 private:
  void Notify() {
    if (waitersHead_ == nullptr) {
      auto head = linkList_.load();
      while (head != 0 && !linkList_.compare_exchange_strong(head, 0)) {
      }
      if (head == 0) return;
      auto cur = reinterpret_cast<ConditionWaitOperation*>(head);
      ConditionWaitOperation* newHead = nullptr;
      while (cur) {
        auto next = cur->next_;
        cur->next_ = newHead;
        newHead = cur;
        cur = next;
      }
      waitersHead_ = newHead;
    }
  }

  std::atomic<uintptr_t> linkList_ = 0;
  ConditionWaitOperation* waitersHead_ = nullptr;
};

}  // namespace Cold

#endif /* COLD_COROUTINES_CONDITION */
