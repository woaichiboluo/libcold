#include "cold/coro/TimeQueue.h"

#include <cassert>

#include "cold/log/Logger.h"

using namespace Cold::Base;

void TimeQueue::AddTimer(Timer& timer) {
  TimerNode node{timer.timerId_, timer.expiry_, timer.GetTaskGenerator(),
                 timer.repeated_, timer.interval_};
  auto it = timerIdToIndexMap_.find(timer.timerId_);
  // not in pendingQueue
  if (it == timerIdToIndexMap_.end()) {
    size_t last = timeHeap_.size();
    timeHeap_.push_back(std::move(node));
    timerIdToIndexMap_[timeHeap_.back().timerId] = last;
    Fixup(last);
  } else {
    // in pendingQueue
    UpdateTimer(timer);
  }
}

void TimeQueue::CancelTimer(Timer& timer) {
  auto it = timerIdToIndexMap_.find(timer.timerId_);
  // timer already complete
  if (it == timerIdToIndexMap_.end()) return;
  auto index = it->second;
  assert(index < timeHeap_.size() &&
         timeHeap_[index].timerId == timer.timerId_);
  timeHeap_[index].Invalid();
}

void TimeQueue::UpdateTimer(Timer& timer) {
  auto it = timerIdToIndexMap_.find(timer.timerId_);
  if (it == timerIdToIndexMap_.end()) {
    return;
  }
  auto index = it->second;
  assert(index < timeHeap_.size() &&
         timeHeap_[index].timerId == timer.timerId_);
  timeHeap_[index].expiry = timer.expiry_;
  auto taskGenerator = timer.GetTaskGenerator();
  if (taskGenerator) {
    timeHeap_[index].taskGenerator = std::move(taskGenerator);
  }
  FixDown(index);
  Fixup(index);
}

void TimeQueue::Fixup(size_t son) {
  for (size_t parent = 0; son > 0;) {
    parent = (son - 1) / 2;
    if (!(timeHeap_[parent] > timeHeap_[son])) break;
    std::swap(timerIdToIndexMap_[timeHeap_[parent].timerId],
              timerIdToIndexMap_[timeHeap_[son].timerId]);
    std::swap(timeHeap_[parent], timeHeap_[son]);
    son = parent;
  }
}

void TimeQueue::FixDown(size_t parent) {
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

int TimeQueue::HandleTimeout(std::vector<Task<>>& timeoutCoroutines) {
  auto now = Time::Now();
  while (!timeHeap_.empty() && now >= timeHeap_[0].expiry) {
    auto& node = timeHeap_[0];
    if (node.valid) {
      timeoutCoroutines.push_back(node.GetTimerTask());
      if (node.repeated) {
        node.expiry += node.interval;
        FixDown(0);
        continue;
      }
    }
    assert(timerIdToIndexMap_.count(node.timerId));
    timerIdToIndexMap_.erase(node.timerId);
    std::swap(timeHeap_.front(), timeHeap_.back());
    if (timeHeap_.size() > 1) timerIdToIndexMap_[timeHeap_.front().timerId] = 0;
    timeHeap_.pop_back();
    FixDown(0);
  }
  assert(timeHeap_.empty() || timeHeap_[0].expiry > now);
  if (!timeHeap_.empty()) {
    auto waitTime = timeHeap_[0].expiry.TimeSinceEpochMilliSeconds() -
                    now.TimeSinceEpochMilliSeconds();
    return waitTime > kDefaultTickMilliSeconds ? kDefaultTickMilliSeconds
                                               : static_cast<int>(waitTime);
  }
  return kDefaultTickMilliSeconds;
}