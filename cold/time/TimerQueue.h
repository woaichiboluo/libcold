#ifndef COLD_TIME_TIMERQUEUE
#define COLD_TIME_TIMERQUEUE

#include <map>
#include <vector>

#include "cold/time/Timer.h"

namespace Cold::Base {
class TimerQueue {
 public:
  TimerQueue() = default;
  ~TimerQueue() = default;

  TimerQueue(const TimerQueue&) = delete;
  TimerQueue& operator=(const TimerQueue&) = delete;

  void AddTimer(Timer& timer);
  void CancelTimer(Timer& timer);
  void UpdateTimer(Timer& timer);

  // return wait time ms
  int HandleTimeout(std::vector<Task<>>& timeoutCoroutines);

 private:
  constexpr static int kDefaultTickMilliSeconds = 500;
  struct TimerNode {
    size_t timerId;
    Time expiry;
    Task<> task;
    bool valid = true;

    bool operator>(const TimerNode& rhs) {
      if (expiry == rhs.expiry) {
        return timerId > rhs.timerId;
      }
      return expiry > rhs.expiry;
    }

    void Invalid() { valid = false; }
  };

  void Fixup(size_t son);
  void FixDown(size_t parent);

  std::vector<TimerNode> timeHeap_;
  std::map<size_t, size_t> timerIdToIndexMap_;
};

}  // namespace Cold::Base

#endif /* COLD_TIME_TIMERQUEUE */