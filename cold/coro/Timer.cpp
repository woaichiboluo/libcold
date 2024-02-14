#include "cold/coro/Timer.h"

#include "cold/coro/IoContext.h"

using namespace Cold::Base;

std::atomic<size_t> Timer::timerCounter = 0;

Timer::Timer(IoContext& ioContext, bool repeated)
    : ioContext_(&ioContext), repeated_(repeated), timerId_(++timerCounter) {}

Timer::~Timer() { Cancel(); }

Timer::Timer(Timer&& other)
    : ioContext_(other.ioContext_),
      expiry_(other.expiry_),
      interval_(other.interval_),
      repeated_(other.repeated_),
      timerId_(other.timerId_) {
  other.ioContext_ = nullptr;
  other.timerId_ = 0;
}

Timer& Timer::operator=(Timer&& other) {
  if (this == &other) return *this;
  assert(ioContext_);
  ioContext_->CancelTimer(*this);
  ioContext_ = other.ioContext_;
  expiry_ = other.expiry_;
  interval_ = other.interval_;
  repeated_ = other.repeated_;
  timerId_ = other.timerId_;
  other.ioContext_ = nullptr;
  other.timerId_ = 0;
  return *this;
}

void Timer::Cancel() {
  if (ioContext_) ioContext_->CancelTimer(*this);
}

void SteadyTimer::ExpiresAt(Time time) {
  expiry_ = time;
  assert(ioContext_);
  ioContext_->UpdateTimer(*this);
}

void SteadyTimer::AsyncWait(Task<> task) {
  task_ = std::move(task);
  assert(ioContext_);
  ioContext_->AddTimer(*this);
}

void RepeatedTimer::SetExpiryAndUpdate() {
  expiry_ = Time::Now() + interval_;
  assert(ioContext_);
  ioContext_->UpdateTimer(*this);
}

void RepeatedTimer::AsyncWait(std::function<Task<>()> taskGenerator) {
  taskGenerator_ = std::move(taskGenerator);
  assert(ioContext_);
  ioContext_->AddTimer(*this);
}