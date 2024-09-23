#ifndef COLD_IO_IOCONTEXT
#define COLD_IO_IOCONTEXT

#include <atomic>
#include <chrono>
#include <memory>

#include "../coroutines/AwaitableBase.h"
#include "../coroutines/Scheduler.h"

namespace Cold {

class Scheduler;
class IoEvent;
class Timer;

namespace detail {
class IoWatcher;
class TimerQueue;

template <typename T, typename REP, typename PERIOD>
requires c_RequireBoth<T>
class Timeout;
}  // namespace detail

class IoContext {
 public:
  IoContext();
  ~IoContext() = default;

  IoContext(const IoContext&) = delete;
  IoContext& operator=(const IoContext&) = delete;

  void Start();
  void Stop();

  bool IsRunning() const { return running_; }

  void CoSpawn(Task<> task);

  // destory task later
  void TaskPendingDone(std::coroutine_handle<> handle);
  // destory task right now
  void TaskDone(std::coroutine_handle<> handle);

  IoEvent* TakeIoEvent(int fd);

  void AddTimer(Timer* timer);
  void UpdateTimer(Timer* timer);
  void CancelTimer(Timer* timer);

  template <typename T, typename REP, typename PERIOD>
  detail::Timeout<T, REP, PERIOD> Timeout(
      T&& value, std::chrono::duration<REP, PERIOD> duration);

 private:
  size_t iterations_ = 0;

  std::atomic<bool> running_ = false;
  std::unique_ptr<Scheduler> scheduler_;
  std::unique_ptr<detail::IoWatcher> ioWatcher_;

  std::mutex mutexForTimerQueue_;
  std::unique_ptr<detail::TimerQueue> timerQueue_;

  std::mutex mutexForPendingTasks_;
  std::vector<Task<>> pendingTasks_;
};

}  // namespace Cold

#endif /* COLD_IO_IOCONTEXT */
