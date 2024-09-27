#ifndef COLD_COROUTINES_SCHEDULER
#define COLD_COROUTINES_SCHEDULER

#include <list>
#include <vector>

#include "../detail/TaskBase.h"

namespace Cold {

class Scheduler {
 public:
  Scheduler() {}
  ~Scheduler() = default;

  Scheduler(const Scheduler&) = delete;
  Scheduler& operator=(const Scheduler&) = delete;

  void CoSpawn(std::coroutine_handle<> handle) {
    allCorotines_.push_back(handle);
    pendingCoros_.push_back(handle);
  }

  void CoResume(std::coroutine_handle<> handle) {
    pendingCoros_.push_back(handle);
  }

  void DoSchedule() {
    for (const auto& handle : pendingCoros_) {
      assert(!handle.done());
      if (!CheckCoroStoped(handle)) handle.resume();
    }
    pendingCoros_.clear();
    allCorotines_.remove_if(IsCoroComplete);
  }

  // for debug
  size_t GetPendingCorosSize() const { return pendingCoros_.size(); }
  size_t GetAllCoroSize() const { return allCorotines_.size(); }

  std::vector<std::coroutine_handle<>>& GetPendingCoros() {
    return pendingCoros_;
  }

 private:
  static bool CheckCoroStoped(std::coroutine_handle<> handle) {
    return std::coroutine_handle<Detail::PromiseBase>::from_address(
               handle.address())
        .promise()
        .GetStopToken()
        .stop_requested();
  }

  static bool IsCoroComplete(std::coroutine_handle<> handle) {
    if (handle.done() || CheckCoroStoped(handle)) {
      handle.destroy();
      return true;
    }
    return false;
  }

  // save all pending coroutines
  std::vector<std::coroutine_handle<>> pendingCoros_;
  // save all coroutines
  std::list<std::coroutine_handle<>> allCorotines_;
};

}  // namespace Cold

#endif /* COLD_COROUTINES_SCHEDULER */
