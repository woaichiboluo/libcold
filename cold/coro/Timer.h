#ifndef COLD_CORO_TIMER
#define COLD_CORO_TIMER

#include "cold/coro/Task.h"
#include "cold/time/Time.h"

namespace Cold::Base {

class IoContext;
class TimeQueue;

class Timer {
  friend class TimeQueue;

 public:
  Timer(IoContext& ioContext, bool repeated);
  virtual ~Timer() = default;

  Timer(const Timer&) = delete;
  Timer& operator=(const Timer&) = delete;

  Time GetExpiry() const { return expiry_; }
  void Cancel();
  bool IsReapted() const { return repeated_; }
  IoContext& GetIoContext() { return ioContext_; }

 protected:
  virtual Task<> GetTimerTask() = 0;
  IoContext& ioContext_;
  Time expiry_;

 private:
  static std::atomic<size_t> timerCounter;
  bool repeated_;
  size_t timerId_;
};

class SteadyTimer : public Timer {
 public:
  SteadyTimer(IoContext& ioContext) : Timer(ioContext, false) {}
  ~SteadyTimer() override = default;

  template <typename Period, typename Rep>
  void ExpiresAfter(std::chrono::duration<Period, Rep> duration) {
    ExpiresAt(Time::Now() + duration);
  }
  void ExpiresAt(Time time);

  void AsyncAwait(Task<> task);

 private:
  Task<> GetTimerTask() override { return std::move(task_); }
  Task<> task_;
};

// class RepeatedTimer : public Timer {};

};  // namespace Cold::Base

#endif /* COLD_CORO_TIMER */
