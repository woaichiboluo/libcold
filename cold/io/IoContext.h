#ifndef COLD_IO_IOCONTEXT
#define COLD_IO_IOCONTEXT

#include <atomic>
#include <list>
#include <memory>

#include "../coroutines/ThisCoro.h"
#include "../detail/IoWatcher.h"
#include "../detail/SigpipeIgnorer.h"
#include "../detail/TimerQueue.h"

namespace Cold {

class IoContext {
 public:
  IoContext()
      : ioWatcher_(std::make_unique<Detail::IoWatcher>(this)),
        timerQueue_(std::make_unique<Detail::TimerQueue>()) {
    Detail::IgnoreSigPipe();
  }

  ~IoContext() {
    for (const auto& handle : unfinishedCoros_) {
      handle.destroy();
    }
  }

  IoContext(const IoContext&) = delete;
  IoContext& operator=(const IoContext&) = delete;

  void Start() {
    assert(!running_);
    running_ = true;
    while (running_) {
      ++iterations_;
      TRACE("IoContext Start of this iterations:{}", iterations_);
      // solve timeout event
      std::vector<Task<>> pendingTasks;
      std::vector<std::coroutine_handle<>> pendingResume;
      {
        std::lock_guard<std::mutex> lock(mutexForTimerQueue_);
        timerQueue_->Tick(pendingTasks);
      }
      DoSchedule(pendingTasks, pendingResume);
      // solve io event
      static constexpr int kDefaultWaitMs = 300;
      auto nextTick = timerQueue_->GetNextTick();
      int waitMs = 0;
      auto now = Time::Now();
      auto waitTime = nextTick.TimeSinceEpochMilliSeconds() -
                      now.TimeSinceEpochMilliSeconds();
      waitMs = static_cast<int>(waitTime);
      waitMs = std::min(waitMs, kDefaultWaitMs);
      ioWatcher_->WatchIo(pendingResume, waitMs);
      DoSchedule(pendingTasks, pendingResume);
      // solve pending event
      {
        std::lock_guard<std::mutex> lock(mutexForPendingTasks_);
        pendingTasks_.swap(pendingTasks);
      }
      {
        std::lock_guard<std::mutex> lock(mutexForPendingResume_);
        pendingResume_.swap(pendingResume);
      }
      DoSchedule(pendingTasks, pendingResume);
      TRACE("IoContext End of this iteration:{}. unfinished coros size: {}",
            iterations_, unfinishedCoros_.size());
    }
  }

  void Stop() {
    running_ = false;
    ioWatcher_->WakeUp();
  }

  bool IsRunning() const { return running_; }

  void CoSpawn(Task<> task) {
    {
      std::lock_guard lock(mutexForPendingTasks_);
      pendingTasks_.push_back(std::move(task));
    }
    ioWatcher_->WakeUp();
  }

  void CoResume(std::coroutine_handle<> handle) {
    {
      std::lock_guard lock(mutexForPendingResume_);
      pendingResume_.push_back(handle);
    }
    ioWatcher_->WakeUp();
  }

  [[nodiscard]] Task<> RunInThisContext() {
    auto& context = co_await ThisCoro::GetCurExecuteIoContext();
    if (&context == this) co_return;
    co_await RunInThisAwaitable(this);
  }

  std::shared_ptr<Detail::IoEvent> TakeIoEvent(int fd) {
    return ioWatcher_->TakeIoEvent(fd);
  }

  void AddTimer(Timer* timer) {
    {
      std::lock_guard<std::mutex> lock(mutexForTimerQueue_);
      timerQueue_->AddTimer(timer);
    }
    ioWatcher_->WakeUp();
  }

  void UpdateTimer(Timer* timer) {
    {
      std::lock_guard<std::mutex> lock(mutexForTimerQueue_);
      timerQueue_->UpdateTimer(timer);
    }
    ioWatcher_->WakeUp();
  }

  void CancelTimer(Timer* timer) {
    {
      std::lock_guard<std::mutex> lock(mutexForTimerQueue_);
      timerQueue_->CancelTimer(timer);
    }
    ioWatcher_->WakeUp();
  }

  size_t GetIterations() const { return iterations_; }

 private:
  class RunInThisAwaitable {
   public:
    RunInThisAwaitable(IoContext* context) : context_(context) {}
    ~RunInThisAwaitable() = default;

    bool await_ready() const noexcept { return false; }

    void await_suspend(std::coroutine_handle<> handle) noexcept {
      context_->CoResume(handle);
    }

    void await_resume() const noexcept {}

   private:
    IoContext* context_;
  };

  void SetCurExecuteContext(std::coroutine_handle<> handle) {
    auto& promise = std::coroutine_handle<Detail::PromiseBase>::from_address(
                        handle.address())
                        .promise();
    promise.SetCurExecuteIoContext(this);
  }

  static bool IsCoroStoped(std::coroutine_handle<> handle) {
    auto& promise = std::coroutine_handle<Detail::PromiseBase>::from_address(
                        handle.address())
                        .promise();
    return promise.GetStopToken().stop_requested();
  }

  static bool Destoryable(std::coroutine_handle<> handle) {
    if (handle.done() || IsCoroStoped(handle)) {
      handle.destroy();
      return true;
    }
    return false;
  }

  void DoSchedule(std::vector<Task<>>& pendingTasks,
                  std::vector<std::coroutine_handle<>>& pendingResume) {
    // solve new task
    for (auto& task : pendingTasks) {
      task.GetHandle().promise().SetIoContext(this);
      auto handle = task.Release();
      unfinishedCoros_.push_back(handle);
      pendingResume.push_back(handle);
    }
    pendingTasks.clear();
    // resume pending coros
    for (auto& handle : pendingResume) {
      if (IsCoroStoped(handle)) continue;
      assert(!handle.done());
      SetCurExecuteContext(handle);
      handle.resume();
    }
    pendingResume.clear();
    // destory finished coros
    unfinishedCoros_.remove_if(Destoryable);
  }

  size_t iterations_ = 0;

  std::atomic<bool> running_ = false;
  std::unique_ptr<Detail::IoWatcher> ioWatcher_;

  std::mutex mutexForTimerQueue_;
  std::unique_ptr<Detail::TimerQueue> timerQueue_;

  std::mutex mutexForPendingTasks_;
  std::vector<Task<>> pendingTasks_;

  std::mutex mutexForPendingResume_;
  std::vector<std::coroutine_handle<>> pendingResume_;

  std::list<std::coroutine_handle<>> unfinishedCoros_;
};

inline void Timer::Add() { context_->AddTimer(this); }
inline void Timer::Update() { context_->UpdateTimer(this); }
inline void Timer::Cancel() {
  if (timerId_ != 0) context_->CancelTimer(this);
}

}  // namespace Cold

#endif /* COLD_IO_IOCONTEXT */
