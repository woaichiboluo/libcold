#ifndef COLD_COROUTINES_SCHEDULER
#define COLD_COROUTINES_SCHEDULER

#include <map>
#include <vector>

#include "Task.h"

namespace Cold {

class Scheduler {
 public:
  Scheduler() = default;
  ~Scheduler() = default;

  Scheduler(const Scheduler&) = delete;
  Scheduler& operator=(const Scheduler&) = delete;

  // for task
  void CoSpawn(Task<> task) {
    auto handle = task.handle_;
    handle.promise().SetScheduler(this);
    tasks_.emplace(handle, std::move(task));
    pendingCoros_.push_back(handle);
  }

  void DoSchedule() {
    for (const auto& handle : pendingCoros_) {
      assert(!handle.done());
      handle.resume();
    }
    pendingCoros_.clear();
    for (const auto& handle : completedTasks_) {
      // not always done,when timeout need to remove the task
      // assert(handle.done());
      assert(tasks_.count(handle));
      tasks_.erase(handle);
    }
    completedTasks_.clear();
  }

  void TaskDone(std::coroutine_handle<> handle) {
    completedTasks_.push_back(handle);
  }

  // for debug
  size_t GetPendingCorosSize() const { return pendingCoros_.size(); }
  size_t GetTasksSize() const { return tasks_.size(); }

  std::vector<std::coroutine_handle<>>& GetPendingCoros() {
    return pendingCoros_;
  }

 private:
  // save all tasks excute by this scheduler
  std::map<std::coroutine_handle<>, Task<>> tasks_;
  // save all pending coroutines
  std::vector<std::coroutine_handle<>> pendingCoros_;
  // excute completed tasks
  std::vector<std::coroutine_handle<>> completedTasks_;
};

namespace detail {

template <typename T>
void PromiseBase::FinalAwaitable::await_suspend(
    std::coroutine_handle<T> handle) noexcept {
  auto& promise = handle.promise();
  if (!promise.continuation_) {
    promise.scheduler_->TaskDone(handle);
  } else {
    auto ready = promise.IsReady();
    promise.SetReady();
    if (ready) promise.continuation_.resume();
  }
}

}  // namespace detail

}  // namespace Cold

#endif /* COLD_COROUTINES_SCHEDULER */
