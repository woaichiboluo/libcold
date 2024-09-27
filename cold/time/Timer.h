#ifndef COLD_TIME_TIMER
#define COLD_TIME_TIMER

#include <atomic>
#include <functional>

#include "../coroutines/Task.h"
#include "Time.h"

namespace Cold {

class IoContext;

namespace Detail {
class TimerQueue;
}

class Timer {
  friend class Detail::TimerQueue;

 public:
  explicit Timer(IoContext& context) : context_(&context) {
    static std::atomic<size_t> timerCreated = 0;
    timerId_ = ++timerCreated;
  }

  ~Timer() { Cancel(); }

  Timer(const Timer&) = delete;
  Timer& operator=(const Timer&) = delete;

  Timer(Timer&& other)
      : context_(other.context_),
        timerId_(other.timerId_),
        expiry_(other.expiry_),
        task_(std::move(other.task_)) {
    other.timerId_ = 0;
  }

  Timer& operator=(Timer&& other) {
    if (this == &other) return *this;
    Cancel();
    context_ = other.context_;
    timerId_ = other.timerId_;
    expiry_ = other.expiry_;
    task_ = std::move(other.task_);
    other.timerId_ = 0;
    return *this;
  }

  IoContext& GetIoContext() const { return *context_; }
  Time GetExpiry() const { return expiry_; }
  size_t GetTimerId() const { return timerId_; }

  void Cancel();

  template <typename REP, typename PERIOD>
  void ExpiresAfter(std::chrono::duration<REP, PERIOD> duration) {
    ExpiresAt(Time::Now() + duration);
  }

  void ExpiresAt(Time time) {
    expiry_ = time;
    Update();
  }

  void AsyncWait(std::function<void()> func) {
    AsyncWait([](std::function<void()> f) -> Task<> {
      f();
      co_return;
    }(std::move(func)));
  }

  void AsyncWait(Task<> task) {
    task_ = std::move(task);
    Add();
  }

  template <typename T>
  struct TimerAwaitable;

  template <typename T>
  [[nodiscard]] TimerAwaitable<T> AsyncWaitable(Task<T> task) {
    return TimerAwaitable<T>(this, std::move(task));
  }

 private:
  void Add();
  void Update();

  IoContext* context_;
  size_t timerId_ = 0;
  Time expiry_;
  Task<> task_;
};

template <typename T>
struct Timer::TimerAwaitable {
 public:
  TimerAwaitable(Timer* timer, Task<T> task)
      : timer_(timer), task_(std::move(task)) {}
  ~TimerAwaitable() = default;

  bool await_ready() noexcept { return false; }

  void await_suspend(std::coroutine_handle<> handle) noexcept {
    auto wrapTask = [](Task<T> task, std::coroutine_handle<> h,
                       T& ret) -> Task<> {
      ret = co_await task;
      h.resume();
    }(std::move(task_), handle, retValue_);
    timer_->AsyncWait(std::move(wrapTask));
  }

  T await_resume() noexcept { return std::move(retValue_); }

 private:
  Timer* timer_;
  Task<T> task_;
  T retValue_;
};

template <>
struct Timer::TimerAwaitable<void> {
 public:
  TimerAwaitable(Timer* timer, Task<> task)
      : timer_(timer), task_(std::move(task)) {}
  ~TimerAwaitable() = default;

  bool await_ready() noexcept { return false; }

  void await_suspend(std::coroutine_handle<> handle) noexcept {
    auto wrapTask = [](Task<> task, std::coroutine_handle<> h) -> Task<> {
      co_await task;
      h.resume();
    }(std::move(task_), handle);
    timer_->AsyncWait(std::move(wrapTask));
  }

  void await_resume() noexcept {}

 private:
  Timer* timer_;
  Task<> task_;
};

}  // namespace Cold

#endif /* COLD_TIME_TIMER */
