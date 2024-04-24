#ifndef COLD_THREAD_CONDITION
#define COLD_THREAD_CONDITION

#include <chrono>

#include "cold/thread/Lock.h"
#include "cold/time/Time.h"

namespace Cold::Base {

class Condition {
 public:
  Condition(Mutex& mutex) : mutex_(&mutex) {
    pthread_cond_init(&cond_, nullptr);
  }

  ~Condition() { pthread_cond_destroy(&cond_); }

  Condition(const Condition&) = delete;
  Condition& operator=(const Condition&) = delete;

  void Wait() { pthread_cond_wait(&cond_, mutex_->NativeHandle()); }

  template <typename Rep, typename Period>
  void WaitFor(std::chrono::duration<Rep, Period> duration) {
    WaitUntil(Time::Now() + duration);
  }

  void WaitUntil(Time time) {
    if (time <= Time::Now()) return;
    auto ts = time.ToTimespec();
    pthread_cond_timedwait(&cond_, mutex_->NativeHandle(), &ts);
  }

  void NotifyOne() { pthread_cond_signal(&cond_); }
  void NotifyAll() { pthread_cond_broadcast(&cond_); }

  pthread_cond_t* NativeHandle() { return &cond_; }

 private:
  Mutex* mutex_;
  pthread_cond_t cond_;
};

}  // namespace Cold::Base

#endif /* COLD_THREAD_CONDITION */
