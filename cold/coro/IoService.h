#ifndef COLD_CORO_IOSERVICE
#define COLD_CORO_IOSERVICE

#include <atomic>
#include <cassert>
#include <coroutine>
#include <map>
#include <memory>
#include <vector>

#include "cold/coro/Task.h"
#include "cold/thread/Lock.h"

namespace Cold::Base {

class IoWatcher;

class IoService {
 public:
  IoService();
  ~IoService();

  IoService(const IoService&) = delete;
  IoService& operator=(const IoService&) = delete;

  void Start();
  void Stop();

  template <typename T>
  void CoSpawn(Task<T> coro) {
    AddTask(WrapTask(std::move(coro)));
  }

  IoWatcher* GetIoWatcher();

 private:
  using Handle = std::coroutine_handle<>;

  struct TaskCompletionAwaitable {
    TaskCompletionAwaitable(IoService* s) : service(s) {}
    bool await_ready() noexcept { return false; }

    Handle await_suspend(Handle handle) noexcept {
      LockGuard guard(service->mutexForCompletionTasks_);
      service->completionTasks_.push_back(handle);
      return handle;
    }

    void await_resume() noexcept {}

    IoService* service;
  };

  template <typename T>
  void WrapTask(Task<T> task);

  void AddTask(Task<> task);

  std::atomic<bool> running_ = false;
  std::unique_ptr<IoWatcher> ioWatcher_;
  const pid_t threadId_;

  Mutex mutexForPendingTasks_;
  std::vector<Task<>> pendingTasks_ GUARDED_BY(mutexForPendingTasks_);

  std::map<Handle, Task<>> awaitCompletionTasks_;

  Mutex mutexForCompletionTasks_;
  std::vector<Handle> completionTasks_ GUARDED_BY(mutexForCompletionTasks_);
};

template <typename T>
void IoService::WrapTask(Task<T> task) {
  return [](Task<T> t, IoService* service) -> Task<> {
    co_await t;
    co_await TaskCompletionAwaitable(service);
  }(std::move(task), this);
}

}  // namespace Cold::Base

#endif /* COLD_CORO_IOSERVICE */
