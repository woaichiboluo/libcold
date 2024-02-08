#include "cold/coro/IoContext.h"

#include <memory>
#include <utility>

#include "cold/coro/IoWatcher-ini.h"
#include "cold/coro/TimeQueue.h"
#include "cold/log/Logger.h"
#include "cold/thread/Thread.h"

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
      assert(task.GetHandle());
      assert(!task.Done());
      auto wrappedTask = [](Task<> coro, IoContext* context) -> Task<> {
        co_await coro;
        co_await TaskCompletionAwaitable(context);
      }(std::move(task), this);
      auto handle = wrappedTask.GetHandle();
      ioContextTasks_.insert({handle, std::move(wrappedTask)});
      handle.resume();
    }
    // execute pending coroutines
    tasks.clear();
    {
      LockGuard guard(mutex_);
      tasks.swap(pendingTasks_);
    }
    for (auto& task : tasks) {
      auto handle = task.GetHandle();
      assert(!handle.done());
      ioContextTasks_.insert({handle, std::move(task)});
      handle.resume();
    }
    // execute wating IO coroutines
    const auto& activeCoros = ioWatcher_->WatchIo(waitTime);
    for (const auto& coro : activeCoros) {
      if (!coro.done()) coro.resume();
    }
    // clear complete coroutines
    std::vector<std::coroutine_handle<>> completions;
    {
      LockGuard guard(mutex_);
      completions.swap(completions_);
    }
    for (const auto& completion : completions) {
      assert(ioContextTasks_.count(completion));
      assert(completion.done());
      ioContextTasks_.erase(completion);
    }
  }
}

void IoContext::Stop() {
  running_ = false;
  ioWatcher_->Wakeup();
}

void IoContext::CoSpawn(Function&& function) {
  if (!function) return;
  AddTask([](Function func, IoContext* context) -> Task<> {
    func();
    co_await TaskCompletionAwaitable(context);
    co_return;
  }(std::move(function), this));
}

void IoContext::AddTask(Task<> task) {
  {
    LockGuard guard(mutex_);
    pendingTasks_.push_back(std::move(task));
  }
  ioWatcher_->Wakeup();
}

void IoContext::HandleIoEvent(internal::IoEvent event) {
  CoSpawn([this, event]() { ioWatcher_->HandleIoEvent(event); });
}

void IoContext::AddReadIoEvent(int fd, std::coroutine_handle<> handle) {
  internal::IoEvent event;
  event.fd = fd;
  event.mode = internal::Mode::READ;
  event.callbackCoroutine = handle;
  HandleIoEvent(event);
}

void IoContext::AddWriteIoEvent(int fd, std::coroutine_handle<> handle) {
  internal::IoEvent event;
  event.fd = fd;
  event.mode = internal::Mode::WRITE;
  event.callbackCoroutine = handle;
  HandleIoEvent(event);
}

void IoContext::RemoveReadIoEvent(int fd) {
  internal::IoEvent event{};
  event.fd = fd;
  event.mode = internal::Mode::DISABLE_READ;
  HandleIoEvent(event);
}

void IoContext::RemoveWriteIoEvent(int fd) {
  internal::IoEvent event{};
  event.fd = fd;
  event.mode = internal::Mode::DISABLE_WRITE;
  HandleIoEvent(event);
}