#ifndef COLD_CORO_TIMER
#define COLD_CORO_TIMER

#include <cassert>
#include <functional>

#include "cold/coro/Task.h"
#include "cold/time/Time.h"

namespace Cold::Base {

class IoContext;
class TimeQueue;

class Timer {
  friend class TimeQueue;

 public:
  Timer(IoContext& ioContext, bool repeated);
  virtual ~Timer();

  Timer(const Timer&) = delete;
  Timer& operator=(const Timer&) = delete;

  Timer(Timer&& other);
  Timer& operator=(Timer&& other);

  Time GetExpiry() const { return expiry_; }
  void Cancel();
  bool IsReapted() const { return repeated_; }
  IoContext* GetIoContext() { return ioContext_; }
  size_t GetTimerId() const { return timerId_; }
  bool Valid() const { return timerId_ > 0; }

 protected:
  virtual std::function<Task<>()> GetTaskGenerator() = 0;
  IoContext* ioContext_;
  Time expiry_;
  std::chrono::milliseconds interval_;

 private:
  static std::atomic<size_t> timerCounter;
  bool repeated_;
  size_t timerId_;
};

namespace internal {
template <typename T>
class SteadyTimerAwaitable;
}

class SteadyTimer : public Timer {
 public:
  SteadyTimer(IoContext& ioContext) : Timer(ioContext, false) {}
  ~SteadyTimer() override = default;

  SteadyTimer(SteadyTimer&&) = default;
  SteadyTimer& operator=(SteadyTimer&&) = default;

  template <typename Period, typename Rep>
  void ExpiresAfter(std::chrono::duration<Period, Rep> duration) {
    ExpiresAt(Time::Now() + duration);
  }
  void ExpiresAt(Time time);

  void AsyncWait(Task<> task);

  template <typename T>
  internal::SteadyTimerAwaitable<T> AsyncWaitable(Task<T> task);

 private:
  std::function<Task<>()> GetTaskGenerator() override {
    return [this]() -> Task<> { return std::move(task_); };
  }
  Task<> task_;
};

namespace internal {

template <typename T>
class SteadyTimerAwaitable : public AwaitableNonCopyable {
 public:
  SteadyTimerAwaitable(SteadyTimer& timer, Task<T> task)
      : timer_(timer), task_(std::move(task)) {}

  bool await_ready() noexcept { return false; }
  void await_suspend(std::coroutine_handle<> handle) noexcept {
    auto newTask = [](Task<T> task, std::coroutine_handle<> h,
                      T& ret) -> Task<void> {
      ret = co_await task;
      h.resume();
    }(std::move(task_), handle, returnValue_);
    timer_.AsyncWait(std::move(newTask));
  }
  auto await_resume() noexcept { return std::move(returnValue_); }

 private:
  SteadyTimer& timer_;
  Task<T> task_;
  T returnValue_;
};

template <>
class SteadyTimerAwaitable<void> : public AwaitableNonCopyable {
 public:
  SteadyTimerAwaitable(SteadyTimer& timer, Task<> task)
      : timer_(timer), task_(std::move(task)) {}

  bool await_ready() noexcept { return false; }
  void await_suspend(std::coroutine_handle<> handle) noexcept {
    auto newTask = [](Task<> task, std::coroutine_handle<> h) -> Task<void> {
      co_await task;
      h.resume();
    }(std::move(task_), handle);
    timer_.AsyncWait(std::move(newTask));
  }
  void await_resume() noexcept {}

 private:
  SteadyTimer& timer_;
  Task<void> task_;
};

template <typename Period, typename Rep>
class SleepAwaitable : public AwaitableNonCopyable {
 public:
  using Duration = std::chrono::duration<Period, Rep>;

  SleepAwaitable(IoContext& context, Duration sleepTime)
      : timer_(context), sleepTime_(sleepTime) {}

  bool await_ready() noexcept { return sleepTime_ <= Duration(); }
  void await_suspend(std::coroutine_handle<> handle) noexcept {
    timer_.ExpiresAfter(sleepTime_);
    timer_.AsyncWait([](std::coroutine_handle<> h) -> Task<void> {
      h.resume();
      co_return;
    }(handle));
  }
  void await_resume() noexcept {}

 private:
  SteadyTimer timer_;
  Duration sleepTime_;
};

}  // namespace internal

template <typename T>
internal::SteadyTimerAwaitable<T> SteadyTimer::AsyncWaitable(Task<T> task) {
  return internal::SteadyTimerAwaitable(*this, std::move(task));
}

template <typename Period, typename Rep>
auto Sleep(IoContext& context, std::chrono::duration<Period, Rep> sleepTime) {
  return internal::SleepAwaitable(context, sleepTime);
}

class RepeatedTimer : public Timer {
 public:
  RepeatedTimer(IoContext& ioContext) : Timer(ioContext, true) {}
  ~RepeatedTimer() override = default;

  template <typename Period, typename Rep>
  void SetInterval(std::chrono::duration<Period, Rep> duration) {
    interval_ = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    assert(interval_ > std::chrono::milliseconds{});
    SetExpiryAndUpdate();
  }

  void AsyncWait(std::function<Task<>()> taskGenerator);

  RepeatedTimer(RepeatedTimer&&) = default;
  RepeatedTimer& operator=(RepeatedTimer&&) = default;

 private:
  void SetExpiryAndUpdate();

  std::function<Task<>()> GetTaskGenerator() override {
    return std::move(taskGenerator_);
  }

  std::function<Task<>()> taskGenerator_;
};

};  // namespace Cold::Base

#endif /* COLD_CORO_TIMER */
