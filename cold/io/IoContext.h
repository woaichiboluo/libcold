#ifndef COLD_IO_IOCONTEXT
#define COLD_IO_IOCONTEXT

#include <atomic>
#include <memory>

#include "../coroutines/Scheduler.h"
#include "../detail/IoWatcher.h"
#include "../detail/SigpipeIgnorer.h"
#include "../detail/TimerQueue.h"

namespace Cold {

class IoContext {
 public:
  IoContext()
      : scheduler_(std::make_unique<Scheduler>()),
        ioWatcher_(std::make_unique<Detail::IoWatcher>(this)),
        timerQueue_(std::make_unique<Detail::TimerQueue>()) {
    Detail::IgnoreSigPipe();
  }

  ~IoContext() = default;

  IoContext(const IoContext&) = delete;
  IoContext& operator=(const IoContext&) = delete;

  void Start() {
    assert(!running_);
    running_ = true;
    while (running_) {
      ++iterations_;
      TRACE("IoContext::Start iterations:{}", iterations_);
      {
        std::lock_guard<std::mutex> lock(mutexForPendingResume_);
        scheduler_->GetPendingCoros().swap(pendingResume_);
      }
      std::vector<Task<>> pendingTasks;
      {
        std::lock_guard<std::mutex> lock(mutexForPendingTasks_);
        pendingTasks.swap(pendingTasks_);
      }
      Time nextTick;
      {
        std::lock_guard<std::mutex> lock(mutexForTimerQueue_);
        nextTick = timerQueue_->Tick(pendingTasks);
      }
      for (auto& task : pendingTasks) {
        task.GetHandle().promise().SetIoContext(this);
        scheduler_->CoSpawn(task.Release());
      }
      scheduler_->DoSchedule();
      int waitMs = 0;
      auto now = Time::Now();
      auto waitTime = nextTick.TimeSinceEpochMilliSeconds() -
                      now.TimeSinceEpochMilliSeconds();
      waitMs = waitTime > 0 ? static_cast<int>(waitTime) : 0;
      static constexpr int kDefaultWaitMs = 300;
      waitMs = std::min(waitMs, kDefaultWaitMs);
      ioWatcher_->WatchIo(scheduler_->GetPendingCoros(), waitMs);
      TRACE(
          "In iteration:{} before DoSchedule pending size: {}, coros size: {}",
          iterations_, scheduler_->GetPendingCorosSize(),
          scheduler_->GetAllCoroSize());
      scheduler_->DoSchedule();
      assert(scheduler_->GetPendingCoros().empty());
    }
  }

  void Stop() {
    running_ = false;
    ioWatcher_->WakeUp();
  }

  bool IsRunning() const { return running_; }

  void CoSpawn(Task<> task) {
    {
      std::lock_guard<std::mutex> lock(mutexForPendingTasks_);
      pendingTasks_.push_back(std::move(task));
    }
    ioWatcher_->WakeUp();
  }

  void CoResume(std::coroutine_handle<> handle) {
    {
      std::lock_guard<std::mutex> lock(mutexForPendingResume_);
      pendingResume_.push_back(handle);
    }
    ioWatcher_->WakeUp();
  }

  std::shared_ptr<Detail::IoEvent> TakeIoEvent(int fd) {
    return ioWatcher_->TakeIoEvent(fd);
  }

  void AddTimer(Timer* timer) {
    std::lock_guard<std::mutex> lock(mutexForTimerQueue_);
    timerQueue_->AddTimer(timer);
  }

  void UpdateTimer(Timer* timer) {
    std::lock_guard<std::mutex> lock(mutexForTimerQueue_);
    timerQueue_->UpdateTimer(timer);
  }

  void CancelTimer(Timer* timer) {
    std::lock_guard<std::mutex> lock(mutexForTimerQueue_);
    timerQueue_->CancelTimer(timer);
  }

  size_t GetIterations() const { return iterations_; }

 private:
  size_t iterations_ = 0;

  std::atomic<bool> running_ = false;
  std::unique_ptr<Scheduler> scheduler_;
  std::unique_ptr<Detail::IoWatcher> ioWatcher_;

  std::mutex mutexForPendingResume_;
  std::vector<std::coroutine_handle<>> pendingResume_;

  std::mutex mutexForTimerQueue_;
  std::unique_ptr<Detail::TimerQueue> timerQueue_;

  std::mutex mutexForPendingTasks_;
  std::vector<Task<>> pendingTasks_;
};

inline void Timer::Add() { context_->AddTimer(this); }
inline void Timer::Update() { context_->UpdateTimer(this); }
inline void Timer::Cancel() {
  if (timerId_ != 0) context_->CancelTimer(this);
}

}  // namespace Cold

#endif /* COLD_IO_IOCONTEXT */
