#ifndef COLD_IO_IOCONTEXT
#define COLD_IO_IOCONTEXT

#include <atomic>
#include <memory>

#include "../coroutines/Scheduler.h"

namespace Cold {

class Scheduler;
class IoEvent;
class Timer;

namespace detail {
class IoWatcher;
class TimerQueue;
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
  void TaskDone(std::coroutine_handle<> handle);

  IoEvent* TakeIoEvent(int fd);

  void AddTimer(Timer* timer);
  void UpdateTimer(Timer* timer);
  void CancelTimer(Timer* timer);

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
