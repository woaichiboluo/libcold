#include "cold/coro/IoService.h"

#include <cassert>
#include <memory>

#include "cold/coro/IoWatcher.h"
#include "cold/log/Logger.h"
#include "cold/thread/Lock.h"
#include "cold/time/TimerQueue.h"

using namespace Cold;

Base::IoService::IoService()
    : ioWatcher_(std::make_unique<IoWatcher>()),
      timerQueue_(std::make_unique<TimerQueue>()) {}

Base::IoService::~IoService() { assert(!running_); }

void Base::IoService::Start() {
  assert(!running_);
  running_ = true;
  while (running_) {
    std::vector<Task<>> tasks;
    // for timer event
    int waitTime = 0;
    {
      LockGuard guard(mutexForTimerQueue_);
      waitTime = timerQueue_->HandleTimeout(tasks);
    }
    for (auto& task : tasks) {
      auto newTask = WrapTask(std::move(task));
      auto handle = newTask.GetHandle();
      awaitCompletionTasks_.insert({handle, std::move(newTask)});
      handle.resume();
    }
    // for io event
    const auto& activeCoros = ioWatcher_->WatchIo(waitTime);
    for (const auto& coro : activeCoros) {
      assert(!coro.done());
      coro.resume();
    }
    // for pendingTasks
    tasks.clear();
    {
      LockGuard guard(mutexForPendingTasks_);
      tasks.swap(pendingTasks_);
    }
    for (auto& task : tasks) {
      auto handle = task.GetHandle();
      awaitCompletionTasks_.insert({handle, std::move(task)});
      handle.resume();
    }
    // clear the completion
    std::vector<Handle> completions;
    {
      LockGuard guard(mutexForCompletionTasks_);
      completions.swap(completionTasks_);
    }
    for (const auto& completion : completions) {
      assert(completion.done());
      awaitCompletionTasks_.erase(completion);
    }
    Base::TRACE("awaitCompletionTasks size: {}", awaitCompletionTasks_.size());
  }
}

void Base::IoService::Stop() {
  running_ = false;
  ioWatcher_->WakeUp();
}

void Base::IoService::AddTask(Task<> task) {
  LockGuard guard(mutexForPendingTasks_);
  pendingTasks_.push_back(std::move(task));
  ioWatcher_->WakeUp();
}

void Base::IoService::AddTimer(Timer& timer) {
  LockGuard guard(mutexForTimerQueue_);
  timerQueue_->AddTimer(timer);
}

void Base::IoService::UpdateTimer(Timer& timer) {
  LockGuard guard(mutexForTimerQueue_);
  timerQueue_->UpdateTimer(timer);
}

void Base::IoService::CancelTimer(Timer& timer) {
  LockGuard guard(mutexForTimerQueue_);
  timerQueue_->CancelTimer(timer);
}

void Base::IoService::ListenReadEvent(int fd, const Handle& handle) {
  ioWatcher_->ListenReadEvent(fd, handle);
}

void Base::IoService::ListenWriteEvent(int fd, const Handle& handle) {
  ioWatcher_->ListenWriteEvent(fd, handle);
}

void Base::IoService::StopListeningReadEvent(int fd) {
  ioWatcher_->StopListeningReadEvent(fd);
}

void Base::IoService::StopListeningWriteEvent(int fd) {
  ioWatcher_->StopListeningWriteEvent(fd);
}

void Base::IoService::StopListeningAll(int fd) {
  ioWatcher_->StopListeningAll(fd);
}