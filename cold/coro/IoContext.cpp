#include "cold/coro/IoContext.h"

#include <coroutine>
#include <memory>
#include <utility>

#include "cold/coro/IoWatcher-ini.h"
#include "cold/coro/TimeQueue.h"
#include "cold/log/Logger.h"
#include "cold/thread/Lock.h"
#include "cold/thread/Thread.h"
#include "cold/time/Time.h"

using namespace Cold::Base;

IoContext::IoContext()
    : ioWatcher_(internal::GetDefaultIoWatcher()),
      timeQueue_(std::make_unique<TimeQueue>()) {}

IoContext::~IoContext() { assert(!running_); }

void IoContext::AddTimer(Timer& timer) {
  assert(&timer.GetIoContext() == this);
  {
    LockGuard guard(timeQueueMutex_);
    timeQueue_->AddTimer(timer);
  }
  ioWatcher_->Wakeup();
}

void IoContext::CancelTimer(Timer& timer) {
  assert(&timer.GetIoContext() == this);
  {
    LockGuard guard(timeQueueMutex_);
    timeQueue_->CancelTimer(timer);
  }
  ioWatcher_->Wakeup();
}

void IoContext::UpdateTimer(Timer& timer) {
  assert(&timer.GetIoContext() == this);
  {
    LockGuard guard(timeQueueMutex_);
    timeQueue_->UpdateTimer(timer);
  }
  ioWatcher_->Wakeup();
}

void IoContext::Start() {
  assert(!running_);
  running_ = true;
  while (running_) {
    int waitTime = 0;
    std::vector<Task<>> tasks;
    {
      LockGuard guard(timeQueueMutex_);
      waitTime = timeQueue_->HandleTimeout(tasks);
    }
    LOG_TRACE(GetMainLogger(), "timeout coroutines count:{} waitTime:{}",
              tasks.size(), waitTime);
    for (auto& task : tasks) {
      auto wrappedTask = [](Task<> coro, IoContext* context) -> Task<> {
        co_await coro;
        co_await TaskCompletionAwaitable(context);
      }(std::move(task), this);
      auto handle = wrappedTask.handle_;
      ioContextTasks_.insert({handle, std::move(wrappedTask)});
      handle.resume();
    }
    // execute wating IO coroutines
    const auto& activeCoros = ioWatcher_->WatchIo(waitTime);
    for (const auto& coro : activeCoros) {
      coro.resume();
    }
    // execute pending coroutines
    tasks.clear();
    {
      LockGuard guard(pendingTasksMutex_);
      tasks.swap(pendingTasks_);
    }
    for (auto& task : tasks) {
      auto handle = task.handle_;
      ioContextTasks_.insert({handle, std::move(task)});
      handle.resume();
    }
    // clear complete coroutines
    for (const auto& completion : completions_) {
      assert(ioContextTasks_.count(completion));
      assert(completion.done());
      ioContextTasks_.erase(completion);
    }
    completions_.clear();
  }
}

void IoContext::Stop() {
  running_ = false;
  ioWatcher_->Wakeup();
}

void IoContext::CoSpawn(Function&& function) {
  AddTask([](Function func, IoContext* context) -> Task<> {
    func();
    co_await TaskCompletionAwaitable(context);
    co_return;
  }(std::move(function), this));
}

void IoContext::AddTask(Task<> task) {
  {
    LockGuard guard(pendingTasksMutex_);
    pendingTasks_.push_back(std::move(task));
  }
  ioWatcher_->Wakeup();
}
