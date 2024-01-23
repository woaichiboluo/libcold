#include "cold/coro/Timer.h"

#include "cold/coro/IoContext.h"

using namespace Cold::Base;

std::atomic<size_t> Timer::timerCounter = 0;

Timer::Timer(IoContext& ioContext, bool repeated)
    : ioContext_(ioContext), repeated_(repeated), timerId_(++timerCounter) {}

Timer::~Timer() { Cancel(); }

void Timer::Cancel() { ioContext_.CancelTimer(*this); }

void SteadyTimer::ExpiresAt(Time time) {
  expiry_ = time;
  ioContext_.UpdateTimer(*this);
}

void SteadyTimer::AsyncWait(Task<> task) {
  task_ = std::move(task);
  ioContext_.AddTimer(*this);
}

void RepeatedTimer::SetExpiryAndUpdate() {
  expiry_ = Time::Now() + interval_;
  ioContext_.UpdateTimer(*this);
}

void RepeatedTimer::AsyncWait(std::function<Task<>()> taskGenerator) {
  taskGenerator_ = std::move(taskGenerator);
  ioContext_.AddTimer(*this);
}