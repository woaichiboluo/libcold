#ifndef COLD_DETAIL_TIMERQUEUE
#define COLD_DETAIL_TIMERQUEUE

#include <map>
#include <vector>

#include "../log/Log.h"
#include "../time/Timer.h"

namespace Cold::Detail {

class TimerQueue {
 public:
  TimerQueue() = default;
  ~TimerQueue() = default;

  TimerQueue(const TimerQueue&) = delete;
  TimerQueue& operator=(const TimerQueue&) = delete;

  void AddTimer(Timer* timer) {
    TimerNode node{timer->timerId_, timer->expiry_, std::move(timer->task_)};
    auto it = timerIdToIndexMap_.find(timer->timerId_);
    if (it == timerIdToIndexMap_.end()) {
      size_t last = timeHeap_.size();
      timeHeap_.push_back(std::move(node));
      timerIdToIndexMap_[timeHeap_.back().timerId] = last;
      Fixup(last);
    } else {
      ERROR("Timer is already in TimerQueue. TimerId: {}", timer->timerId_);
    }
  }

  void CancelTimer(Timer* timer) {
    auto it = timerIdToIndexMap_.find(timer->timerId_);
    if (it == timerIdToIndexMap_.end()) return;
    auto index = it->second;
    assert(index < timeHeap_.size() &&
           timeHeap_[index].timerId == timer->timerId_);
    timeHeap_[index].Invalid();
  }

  void UpdateTimer(Timer* timer) {
    auto it = timerIdToIndexMap_.find(timer->timerId_);
    if (it == timerIdToIndexMap_.end()) {
      return;
    }
    auto index = it->second;
    assert(index < timeHeap_.size() &&
           timeHeap_[index].timerId == timer->timerId_);
    // update expiry time
    timeHeap_[index].expiry = timer->expiry_;
    FixDown(index);
    Fixup(index);
  }

  void Tick(std::vector<Task<>>& timeoutTasks) {
    auto now = Time::Now();
    while (!timeHeap_.empty() && now >= timeHeap_[0].expiry) {
      auto& node = timeHeap_[0];
      if (node.valid) {
        timeoutTasks.push_back(std::move(node.task));
      }
      assert(timerIdToIndexMap_.count(node.timerId));
      timerIdToIndexMap_.erase(node.timerId);
      std::swap(timeHeap_.front(), timeHeap_.back());
      if (timeHeap_.size() > 1)
        timerIdToIndexMap_[timeHeap_.front().timerId] = 0;
      timeHeap_.pop_back();
      FixDown(0);
    }
    assert(timeHeap_.empty() || timeHeap_[0].expiry > now);
  }

  Time GetNextTick() {
    if (!timeHeap_.empty()) {
      return timeHeap_[0].expiry;
    }
    return Time::Now() + std::chrono::seconds(1);
  }

 private:
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

  void Fixup(size_t son) {
    for (size_t parent = 0; son > 0;) {
      parent = (son - 1) / 2;
      if (!(timeHeap_[parent] > timeHeap_[son])) break;
      std::swap(timerIdToIndexMap_[timeHeap_[parent].timerId],
                timerIdToIndexMap_[timeHeap_[son].timerId]);
      std::swap(timeHeap_[parent], timeHeap_[son]);
      son = parent;
    }
  }

  void FixDown(size_t parent) {
    auto last = timeHeap_.size();
    for (auto son = parent * 2 + 1; son < last; son = parent * 2 + 1) {
      if (son + 1 < last && timeHeap_[son] > timeHeap_[son + 1]) ++son;
      if (timeHeap_[son] > timeHeap_[parent]) break;
      std::swap(timerIdToIndexMap_[timeHeap_[parent].timerId],
                timerIdToIndexMap_[timeHeap_[son].timerId]);
      std::swap(timeHeap_[parent], timeHeap_[son]);
      parent = son;
    }
  }

  std::vector<TimerNode> timeHeap_;
  std::map<size_t, size_t> timerIdToIndexMap_;
};

}  // namespace Cold::Detail

#endif /* COLD_DETAIL_TIMERQUEUE */
