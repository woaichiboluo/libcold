#include "cold/coro/IoService.h"

#include <cassert>
#include <memory>

#include "cold/coro/IoWatcher.h"
#include "cold/log/Logger.h"
#include "cold/thread/Lock.h"
#include "cold/thread/Thread.h"

using namespace Cold;

Base::IoService::IoService()
    : ioWatcher_(std::make_unique<IoWatcher>()),
      threadId_(ThisThread::ThreadId()) {}

Base::IoService::~IoService() { assert(!running_); }

void Base::IoService::Start() {
  assert(!running_);
  running_ = true;
  while (running_) {
    int waitTime = 500;
    const auto& activeCoros = ioWatcher_->WatchIo(waitTime);
    for (const auto& coro : activeCoros) {
      assert(!coro.done());
      coro.resume();
    }
    std::vector<Task<>> tasks;
    {
      LockGuard guard(mutexForPendingTasks_);
      tasks.swap(pendingTasks_);
    }
    for (auto& task : tasks) {
      auto handle = task.handle_;
      awaitCompletionTasks_.insert({handle, std::move(task)});
      handle.resume();
    }
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

Base::IoWatcher* Base::IoService::GetIoWatcher() { return ioWatcher_.get(); }