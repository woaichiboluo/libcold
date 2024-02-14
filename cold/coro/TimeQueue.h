#ifndef COLD_CORO_TIMEQUEUE
#define COLD_CORO_TIMEQUEUE

#include <functional>
#include <map>

#include "cold/coro/Task.h"
#include "cold/coro/Timer.h"

namespace Cold::Base {

class TimeQueue {
 public:
  TimeQueue() = default;
  ~TimeQueue() = default;

  TimeQueue(const TimeQueue&) = delete;
  TimeQueue& operator=(const TimeQueue&) = delete;

  void AddTimer(Timer& timer);
  void CancelTimer(Timer& timer);
  void UpdateTimer(Timer& timer);

  // return wait time
  int HandleTimeout(std::vector<Task<>>& timeoutCoroutines);

 private:
  constexpr static int kDefaultTickMilliSeconds = 500;
  struct TimerNode {
    size_t timerId;
    Time expiry;
    std::function<Task<>()> taskGenerator;
    bool repeated;
    std::chrono::milliseconds interval;
    bool valid = true;

    // void swap(TimerNode& other) {
    //   std::swap(timerId, other.timerId);
    //   std::swap(expiry, other.expiry);
    //   taskGenerator.swap(other.taskGenerator);
    //   std::swap(repeated, other.repeated);
    //   std::swap(interval, other.interval);
    //   std::swap(valid, other.valid);
    // }

    bool operator>(const TimerNode& rhs) {
      if (expiry == rhs.expiry) {
        return timerId < rhs.timerId;
      }
      return expiry > rhs.expiry;
    }

    Task<> GetTimerTask() { return taskGenerator(); }
    void Invalid() { valid = false; }
  };

  void Fixup(size_t son);
  void FixDown(size_t parent);

  std::vector<TimerNode> timeHeap_;
  std::map<size_t, size_t> timerIdToIndexMap_;
  std::vector<Task<>> timeoutCorotines_;
};

}  // namespace Cold::Base
#endif /* COLD_CORO_TIMEQUEUE */
