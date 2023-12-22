#ifndef COLD_THREAD_CONDITION
#define COLD_THREAD_CONDITION

#include <chrono>

#include "cold/thread/Lock.h"
#include "cold/time/Time.h"

namespace Cold::Base {

class Condition {
 public:
  Condition(Mutex& mutex) : mutex_(mutex) {
    pthread_cond_init(&cond_, nullptr);
  }

  ~Condition() { pthread_cond_destroy(&cond_); }

  Condition(const Condition&) = delete;
  Condition& operator=(const Condition&) = delete;

  void wait() { pthread_cond_wait(&cond_, mutex_.nativeHandle()); }

  template <typename Duration>
  void waitFor(Duration duration) {
    waitUntil(Time::now() + duration);
  }

  void waitUntil(Time time) {
    if (time <= Time::now()) return;
    auto ts = time.toTimespec();
    pthread_cond_timedwait(&cond_, mutex_.nativeHandle(), &ts);
  }

  void notifyOne() { pthread_cond_signal(&cond_); }
  void notifyAll() { pthread_cond_broadcast(&cond_); }

  pthread_cond_t* nativeHandle() { return &cond_; }

 private:
  Mutex& mutex_;
  pthread_cond_t cond_;
};

}  // namespace Cold::Base

#endif /* COLD_THREAD_CONDITION */
