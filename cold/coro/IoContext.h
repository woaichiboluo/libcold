#ifndef COLD_CORO_IOCONTEXT
#define COLD_CORO_IOCONTEXT

#include <coroutine>
#include <functional>
#include <map>

#include "cold/coro/Task.h"
#include "cold/thread/Lock.h"

namespace Cold::Base {

class IoWatcher;
class TimeQueue;
class Timer;

class IoContext {
 public:
  using Function = std::function<void()>;

  IoContext();
  ~IoContext();

  void CoSpawn(Function&& function);
  template <typename T>
  void CoSpawn(Task<T>&& task);

  void AddTimer(Timer& timer);
  void CancelTimer(Timer& timer);
  void UpdateTimer(Timer& timer);

  void Start();
  void Stop();

 private:
  struct TaskCompletionAwaitable {
    using promise_type = Task<>::promise_type;

    TaskCompletionAwaitable(IoContext* context) : ioContext(context) {}

    bool await_ready() noexcept { return false; }
    void await_suspend(std::coroutine_handle<promise_type> handle) noexcept {
      assert(ioContext);
      ioContext->completions_.push_back(handle);
      handle.resume();
    }
    void await_resume() noexcept {}

    IoContext* ioContext;
  };

  void AddTask(Task<> task);

  std::unique_ptr<IoWatcher> ioWatcher_;
  Mutex timeQueueMutex_;
  std::unique_ptr<TimeQueue> timeQueue_ GUARDED_BY(timeQueueMutex_);
  Mutex pendingTasksMutex_;
  std::vector<Task<>> pendingTasks_ GUARDED_BY(pendingTasksMutex_);
  std::map<std::coroutine_handle<>, Task<>> ioContextTasks_;
  std::vector<std::coroutine_handle<>> completions_;
  std::atomic<bool> running_ = false;
};

template <typename T>
void IoContext::CoSpawn(Task<T>&& task) {
  AddTask([](Task<T> coro, IoContext* context) -> Task<> {
    co_await coro;
    co_await TaskCompletionAwaitable(context);
  }(std::move(task), this));
}

}  // namespace Cold::Base

#endif /* COLD_CORO_IOCONTEXT */
