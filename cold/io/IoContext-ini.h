#ifndef COLD_IO_IOCONTEXT_INI
#define COLD_IO_IOCONTEXT_INI

#include <sys/signal.h>

#include "../time/TimeAwaitable.h"
#include "../time/TimerQueue.h"
#include "IoContext.h"
#include "IoWatcher-ini.h"

namespace Cold {

namespace detail {
class IgnoreSigPipe {
 public:
  IgnoreSigPipe() {
    ::signal(SIGPIPE, SIG_IGN);
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    sigprocmask(SIG_BLOCK, &set, nullptr);
  }
};

inline void _IgnoreSigPipe() { static IgnoreSigPipe __; }

}  // namespace detail

inline IoContext::IoContext()
    : scheduler_(std::make_unique<Scheduler>()),
      ioWatcher_(std::make_unique<detail::IoWatcher>(this)),
      timerQueue_(std::make_unique<detail::TimerQueue>()) {
  detail::IgnoreSigPipe();
}

inline void IoContext::Start() {
  iterations_ = 0;
  running_ = true;
  while (running_) {
    ++iterations_;
    TRACE("IoContext::Start iterations:{}", iterations_);
    std::vector<Task<>> pendingTasks;
    {
      std::lock_guard<std::mutex> lock(mutexForPendingTasks_);
      pendingTasks.swap(pendingTasks_);
    }
    int waitMs = 0;
    {
      std::lock_guard<std::mutex> lock(mutexForTimerQueue_);
      waitMs = timerQueue_->Tick(pendingTasks);
    }
    for (auto& task : pendingTasks) {
      scheduler_->CoSpawn(std::move(task));
    }
    TRACE("In iteration:{} Schedule Stage-1 pendings size: {} tasks size: {}",
          iterations_, scheduler_->GetPendingCorosSize(),
          scheduler_->GetTasksSize());
    scheduler_->DoSchedule();
    ioWatcher_->WatchIo(scheduler_->GetPendingCoros(), waitMs);
    TRACE("In iteration:{} Schedule Stage-2 pendings size: {} tasks size: {}",
          iterations_, scheduler_->GetPendingCorosSize(),
          scheduler_->GetTasksSize());
    scheduler_->DoSchedule();
  }
}

inline void IoContext::Stop() {
  assert(running_);
  running_ = false;
  ioWatcher_->WakeUp();
}

inline void IoContext::CoSpawn(Task<> task) {
  {
    std::lock_guard<std::mutex> lock(mutexForPendingTasks_);
    pendingTasks_.push_back(std::move(task));
  }
  ioWatcher_->WakeUp();
}

inline void IoContext::TaskPendingDone(std::coroutine_handle<> handle) {
  scheduler_->TaskPendingDone(handle);
}

inline void IoContext::TaskDone(std::coroutine_handle<> handle) {
  scheduler_->TaskDone(handle);
}

inline IoEvent* IoContext::TakeIoEvent(int fd) {
  return ioWatcher_->TakeIoEvent(fd);
}

inline void IoContext::AddTimer(Timer* timer) {
  std::lock_guard lock(mutexForTimerQueue_);
  timerQueue_->AddTimer(timer);
}

inline void IoContext::UpdateTimer(Timer* timer) {
  std::lock_guard lock(mutexForTimerQueue_);
  timerQueue_->UpdateTimer(timer);
}

inline void IoContext::CancelTimer(Timer* timer) {
  std::lock_guard lock(mutexForTimerQueue_);
  assert(timerQueue_);
  timerQueue_->CancelTimer(timer);
}

template <typename T, typename REP, typename PERIOD>
detail::Timeout<T, REP, PERIOD> IoContext::Timeout(
    T&& value, std::chrono::duration<REP, PERIOD> duration) {
  return detail::Timeout<T, REP, PERIOD>(*this, std::forward<T>(value),
                                         duration);
}

}  // namespace Cold

#endif /* COLD_IO_IOCONTEXT_INI */
