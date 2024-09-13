#ifndef COLD_THREAD_THREAD
#define COLD_THREAD_THREAD

#include <string.h>

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <functional>
#include <thread>

namespace Cold {

namespace ThisThread {

inline const pid_t& ThreadId() {
  static thread_local pid_t tid = gettid();
  return tid;
}

inline const std::string& ThreadIdStr() {
  static thread_local std::string tidStr = std::to_string(ThreadId());
  return tidStr;
}

inline const std::string& ThreadName() {
  static thread_local std::string threadName = "main";
  return threadName;
}

inline const char* ErrorMsg() {
  static thread_local char errorMsg[256];
  return strerror_r(errno, errorMsg, sizeof(errorMsg));
}

}  // namespace ThisThread

class Thread {
 public:
  Thread(std::function<void()> job, std::string name = "")
      : job_(std::move(job)),
        name_(std::move(name)),
        mutex_(std::make_unique<std::mutex>()),
        condition_(std::make_unique<std::condition_variable>()) {
    static std::atomic<uint32_t> numCreated = 0;
    if (name_.empty()) {
      name_ = "Thread-" + std::to_string(++numCreated);
    }
  }

  ~Thread() {
    if (thread_ && thread_->joinable()) thread_->detach();
  }

  Thread(const Thread&) = delete;
  Thread& operator=(const Thread&) = delete;

  Thread(Thread&&) = default;
  Thread& operator=(Thread&&) = default;

  const std::string& GetName() const { return name_; }

  void Start() {
    assert(!started_);
    assert(!Started());
    thread_ = std::make_unique<std::thread>([job = std::move(job_), this]() {
      {
        std::unique_lock lock(*mutex_);
        ThisThread::ThreadId();
        const_cast<std::string&>(ThisThread::ThreadName()) = name_;
        started_ = true;
        condition_->notify_all();
      }
      // init thread name and thread id
      job();
    });
    std::unique_lock lock(*mutex_);
    condition_->wait(lock, [this] { return started_; });
  }

  bool Started() const { return thread_ != nullptr && started_; }

  void Join() {
    assert(Joinable());
    thread_->join();
  }

  void Detach() {
    assert(Joinable());
    thread_->detach();
  }

  bool Joinable() const { return thread_ && thread_->joinable(); }

 private:
  std::function<void()> job_;
  std::string name_;
  std::unique_ptr<std::mutex> mutex_;
  std::unique_ptr<std::condition_variable> condition_;
  std::unique_ptr<std::thread> thread_;
  bool started_ = false;
};

}  // namespace Cold

#endif /* COLD_THREAD_THREAD */
