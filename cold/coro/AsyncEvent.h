#ifndef COLD_CORO_ASYNCEVENT
#define COLD_CORO_ASYNCEVENT

#include <atomic>
#include <cassert>
#include <coroutine>

// modify from cppcoro/single_consumer_async_auto_reset_event.hpp

namespace Cold::Base {

class AsyncEvent {
 public:
  AsyncEvent(bool initiallySet = false) noexcept
      : state_(initiallySet ? this : nullptr) {}

  ~AsyncEvent() = default;

  AsyncEvent(const AsyncEvent&) = delete;
  AsyncEvent& operator=(const AsyncEvent&) = delete;

  void Set() noexcept {
    void* oldValue = state_.exchange(this, std::memory_order_release);
    if (oldValue != nullptr && oldValue != this) {
      auto handle = *static_cast<std::coroutine_handle<>*>(oldValue);
      (void)state_.exchange(nullptr, std::memory_order_acquire);
      handle.resume();
    }
  }

  auto operator co_await() const noexcept {
    class awaiter {
     public:
      awaiter(const AsyncEvent& event) noexcept : event_(event) {}

      bool await_ready() const noexcept { return false; }

      bool await_suspend(std::coroutine_handle<> awaitingCoroutine) noexcept {
        awaitingCoroutine_ = awaitingCoroutine;
        void* oldValue = nullptr;
        if (!event_.state_.compare_exchange_strong(
                oldValue, &awaitingCoroutine_, std::memory_order_release,
                std::memory_order_relaxed)) {
          assert(oldValue == &event_);
          (void)event_.state_.exchange(nullptr, std::memory_order_acquire);
          return false;
        }

        return true;
      }

      void await_resume() noexcept {}

     private:
      const AsyncEvent& event_;
      std::coroutine_handle<> awaitingCoroutine_;
    };

    return awaiter{*this};
  }

 private:
  // nullptr - not set, no waiter
  // this    - set
  // other   - not set, pointer is address of a coroutine_handle<> to resume.
  mutable std::atomic<void*> state_;
};

}  // namespace Cold::Base

#endif /* COLD_CORO_ASYNCEVENT */
