#include "cold/thread/Thread.h"

#include <unistd.h>

#include <cstring>

#include "cold/thread/Condition.h"
#include "cold/thread/Lock.h"

using namespace Cold;

namespace {
thread_local pid_t t_threadId = 0;
thread_local std::string t_threadIdStr;
thread_local std::string t_threadName = "main";
thread_local char t_errorMsg[256];
}  // namespace

void CacheTidAndTidStr() {
  t_threadId = gettid();
  t_threadIdStr = std::to_string(t_threadId);
}

pid_t Base::ThisThread::ThreadId() {
  if (t_threadId == 0) CacheTidAndTidStr();
  return t_threadId;
}

const std::string& Base::ThisThread::ThreadIdStr() {
  if (t_threadId == 0) CacheTidAndTidStr();
  return t_threadIdStr;
}

const std::string& Base::ThisThread::ThreadName() { return t_threadName; }

const char* Base::ThisThread::ErrorMsg() {
  return strerror_r(errno, t_errorMsg, sizeof(t_errorMsg));
}

std::atomic<int> Base::Thread::numCreated_ = 0;

Base::Thread::Thread(ThreadTask task, std::string threadName)
    : task_(std::move(task)),
      threadName_(std::move(threadName)),
      condition_(mutex_) {
  ++numCreated_;
  if (threadName_.empty()) {
    threadName_ = "Thread-" + std::to_string(numCreated_.load());
  }
}

Base::Thread::~Thread() {
  if (Joinable()) {
    Detach();
  }
}

void Base::Thread::Start() {
  assert(!started_);
  LockGuard guard(mutex_);
  pthread_create(&thread_, nullptr, &Thread::ThreadMainFunc, this);
  while (!started_) {
    condition_.Wait();
  }
  assert(started_);
}

void Base::Thread::Join() {
  assert(Joinable());
  pthread_join(thread_, nullptr);
  joined_ = true;
}

void Base::Thread::Detach() {
  assert(Joinable());
  pthread_detach(thread_);
  joined_ = true;
}

void* Base::Thread::ThreadMainFunc(void* arg) {
  auto self = static_cast<Thread*>(arg);
  CacheTidAndTidStr();
  self->threadId_ = t_threadId;
  t_threadName = self->threadName_;
  {
    LockGuard guard(self->mutex_);
    self->started_ = true;
    self->condition_.NotifyOne();
  }
  auto task = std::move(self->task_);
  if (task) task();
  return nullptr;
}