#include "cold/time/TimerQueue.h"

#include <cassert>

#include "cold/log/Logger.h"

using namespace Cold;

void Base::TimerQueue::AddTimer(Timer& timer) {
  TimerNode node{timer.timerId_, timer.expiry_, std::move(timer.task_)};
  auto it = timerIdToIndexMap_.find(timer.timerId_);
  if (it == timerIdToIndexMap_.end()) {
    size_t last = timeHeap_.size();
    timeHeap_.push_back(std::move(node));
    timerIdToIndexMap_[timeHeap_.back().timerId] = last;
    Fixup(last);
  } else {
    Base::ERROR("Timer is already in TimerQueue. TimerId: {}", timer.timerId_);
  }
}

void Base::TimerQueue::CancelTimer(Timer& timer) {
  auto it = timerIdToIndexMap_.find(timer.timerId_);
  // timer already complete
  if (it == timerIdToIndexMap_.end()) return;
  auto index = it->second;
  assert(index < timeHeap_.size() &&
         timeHeap_[index].timerId == timer.timerId_);
  timeHeap_[index].Invalid();
}

void Base::TimerQueue::UpdateTimer(Timer& timer) {
  auto it = timerIdToIndexMap_.find(timer.timerId_);
  if (it == timerIdToIndexMap_.end()) {
    return;
  }
  auto index = it->second;
  assert(index < timeHeap_.size() &&
         timeHeap_[index].timerId == timer.timerId_);
  // update expiry time
  timeHeap_[index].expiry = timer.expiry_;
  FixDown(index);
  Fixup(index);
}

void Base::TimerQueue::Fixup(size_t son) {
  for (size_t parent = 0; son > 0;) {
    parent = (son - 1) / 2;
    if (!(timeHeap_[parent] > timeHeap_[son])) break;
    std::swap(timerIdToIndexMap_[timeHeap_[parent].timerId],
              timerIdToIndexMap_[timeHeap_[son].timerId]);
    std::swap(timeHeap_[parent], timeHeap_[son]);
    son = parent;
  }
}

void Base::TimerQueue::FixDown(size_t parent) {
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

int Base::TimerQueue::HandleTimeout(std::vector<Task<>>& timeoutCoroutines) {
  auto now = Time::Now();
  while (!timeHeap_.empty() && now >= timeHeap_[0].expiry) {
    auto& node = timeHeap_[0];
    if (node.valid) {
      timeoutCoroutines.push_back(std::move(node.task));
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