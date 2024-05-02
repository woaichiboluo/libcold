#include "cold/time/Timer.h"

#include "cold/coro/IoService.h"

using namespace Cold;

size_t Base::Timer::numCreated_ = 0;

Base::Timer::Timer(IoService& service)
    : service_(&service), timerId_(++numCreated_) {}

Base::Timer::~Timer() { Cancel(); }

void Base::Timer::Timer::ExpiresAt(Time time) {
  expiry_ = time;
  service_->UpdateTimer(*this);
}

void Base::Timer::Timer::AsyncWait(Task<> task) {
  task_ = std::move(task);
  service_->AddTimer(*this);
}

void Base::Timer::Timer::Cancel() { service_->CancelTimer(*this); }