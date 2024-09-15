#ifndef COLD_IO_IOCONTEXT_INI
#define COLD_IO_IOCONTEXT_INI

#include <sys/signal.h>

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
      ioWatcher_(std::make_unique<IoWatcher>(this)) {
  detail::IgnoreSigPipe();
}

inline void IoContext::Start() {
  running_ = true;
  while (running_) {
    std::vector<Task<>> pendingTasks;
    {
      std::lock_guard<std::mutex> lock(mutexForPendingTasks_);
      pendingTasks.swap(pendingTasks_);
    }
    for (auto& task : pendingTasks) {
      scheduler_->CoSpawn(std::move(task));
    }
    ioWatcher_->WatchIo(scheduler_->GetPendingCoros(), 500);
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

inline IoEvent* IoContext::TakeIoEvent(int fd) {
  return ioWatcher_->TakeIoEvent(fd);
}

}  // namespace Cold

#endif /* COLD_IO_IOCONTEXT_INI */
