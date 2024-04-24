#ifndef COLD_THREAD_THREAD
#define COLD_THREAD_THREAD

#include <atomic>
#include <functional>
#include <string>

#include "cold/thread/Condition.h"
#include "cold/thread/Lock.h"
namespace Cold::Base {

namespace ThisThread {
pid_t ThreadId();
const std::string& ThreadIdStr();
const std::string& ThreadName();
const char* ErrorMsg();
}  // namespace ThisThread

class Thread {
 public:
  using ThreadTask = std::function<void()>;

  Thread(ThreadTask, std::string threadName = "");
  ~Thread();

  Thread(const Thread&) = delete;
  Thread& operator=(const Thread&) = delete;

  void Start();
  void Join();
  void Detach();

  bool Started() const { return started_; }
  bool Joinable() const { return started_ && !joined_; }

  pid_t GetThreadId() const { return threadId_; }
  const std::string& GetThreadName() const { return threadName_; }

 private:
  static void* ThreadMainFunc(void* arg);
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
