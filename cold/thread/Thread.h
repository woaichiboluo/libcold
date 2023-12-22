#ifndef COLD_THREAD_THREAD
#define COLD_THREAD_THREAD

#include <atomic>
#include <functional>
#include <string>

#include "cold/thread/Condition.h"
#include "cold/thread/Lock.h"
namespace Cold::Base {

namespace ThisThread {
pid_t threadId();
const std::string& threadIdStr();
const std::string& threadName();
}  // namespace ThisThread

class Thread {
 public:
  using ThreadTask = std::function<void()>;

  Thread(ThreadTask, std::string threadName = "");
  ~Thread();

  Thread(const Thread&) = delete;
  Thread& operator=(const Thread&) = delete;

  void start();
  void join();
  void detach();

  bool started() const { return started_; }
  bool joinable() const { return started_ && !joined_; }

  pid_t getThreadId() const { return threadId_; }
  const std::string& getThreadName() const { return threadName_; }

 private:
  static void* threadMainFunc(void* arg);
  static std::atomic<int> numCreated_;

  ThreadTask task_;
  std::string threadName_;
  pthread_t thread_;

  Mutex mutex_;
  Condition condition_;

  std::atomic<bool> started_ = false;
  std::atomic<bool> joined_ = false;
  pid_t threadId_ = 0;
};

}  // namespace Cold::Base

#endif /* COLD_THREAD_THREAD */
