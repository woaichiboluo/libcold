#include "cold/coro/Timer.h"

#include "cold/coro/IoContext.h"

using namespace Cold::Base;

std::atomic<size_t> Timer::timerCounter = 0;

Timer::Timer(IoContext& ioContext, bool repeated)
    : ioContext_(ioContext), repeated_(repeated), timerId_(++timerCounter) {}

void Timer::Cancel() { ioContext_.CancelTimer(*this); }

void SteadyTimer::ExpiresAt(Time time) {
  expiry_ = time;
  ioContext_.UpdateTimer(*this);
}

void SteadyTimer::AsyncAwait(Task<> task) {
  task_ = std::move(task);
  ioContext_.AddTimer(*this);
}