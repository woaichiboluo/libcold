#ifndef COLD_TIME_TIMER
#define COLD_TIME_TIMER

#include <chrono>
#include <coroutine>

#include "cold/coro/Task.h"
#include "cold/time/Time.h"

namespace Cold::Base {

class IoService;

class Timer {
  friend class TimerQueue;

 public:
  explicit Timer(IoService& service);
  ~Timer();

  Timer(const Timer&) = delete;
  Timer& operator=(const Timer&) = delete;

  Timer(Timer&& other)
      : service_(other.service_),
        timerId_(other.timerId_),
        expiry_(other.expiry_),
        task_(std::move(other.task_)) {
    other.timerId_ = 0;
  }

  Timer& operator=(Timer&& other) {
    if (this == &other) {
      return *this;
    }
    Cancel();
    service_ = other.service_;
    timerId_ = other.timerId_;
    expiry_ = other.expiry_;
    task_ = std::move(other.task_);
    other.timerId_ = 0;
    return *this;
  }

  template <typename T>
  struct TimerAwaitable;

  template <typename REP, typename PERIOD>
  void ExpiresAfter(std::chrono::duration<REP, PERIOD> duration) {
    ExpiresAt(Time::Now() + duration);
  }

  void ExpiresAt(Time time);

  void AsyncWait(Task<> task);

  Time GetExpiry() const { return expiry_; }

  size_t GetTimerId() const { return timerId_; }

  void Cancel();

  template <typename T>
  TimerAwaitable<T> AsyncWaitable(Task<T> task);

 private:
  static size_t numCreated_;

  IoService* service_;
  size_t timerId_;
  Time expiry_;
  Task<> task_;
};

template <typename T>
struct Timer::TimerAwaitable {
 public:
  TimerAwaitable(Timer& timer, Task<T> task)
      : timer_(&timer), task_(std::move(task)) {}
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
  TimerAwaitable(Timer& timer, Task<> task)
      : timer_(&timer), task_(std::move(task)) {}
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

template <typename T>
Timer::TimerAwaitable<T> Timer::AsyncWaitable(Task<T> task) {
  return TimerAwaitable(*this, std::move(task));
}

}  // namespace Cold::Base

#endif /* COLD_TIME_TIMER */
