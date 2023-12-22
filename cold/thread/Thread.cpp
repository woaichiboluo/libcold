#include "cold/thread/Thread.h"

#include <unistd.h>

#include <cassert>

#include "cold/thread/Condition.h"
#include "cold/thread/Lock.h"
#include "cold/util/StringUtil.h"

using namespace Cold;

thread_local pid_t t_threadId = 0;
thread_local std::string t_threadIdStr;
thread_local std::string t_threadName = "main";

void cacheTidAndTidStr() {
  t_threadId = gettid();
  t_threadIdStr = Base::intToStr(t_threadId);
}

pid_t Base::ThisThread::threadId() {
  if (t_threadId == 0) cacheTidAndTidStr();
  return t_threadId;
}

const std::string& Base::ThisThread::threadIdStr() {
  if (t_threadId == 0) cacheTidAndTidStr();
  return t_threadIdStr;
}

const std::string& Base::ThisThread::threadName() { return t_threadName; }

std::atomic<int> Base::Thread::numCreated_ = 0;

Base::Thread::Thread(ThreadTask task, std::string threadName)
    : task_(std::move(task)),
      threadName_(std::move(threadName)),
      condition_(mutex_) {
  ++numCreated_;
  if (threadName_.empty()) {
    threadName_ = "Thread" + intToStr(numCreated_.load());
  }
}

Base::Thread::~Thread() {
  if (joinable()) {
    detach();
  }
}

void Base::Thread::start() {
  assert(!started_);
  LockGuard guard(mutex_);
  pthread_create(&thread_, nullptr, &Thread::threadMainFunc, this);
  while (!started_) {
    condition_.wait();
  }
  assert(started_);
}

void Base::Thread::join() {
  assert(joinable());
  pthread_join(thread_, nullptr);
  joined_ = true;
}

void Base::Thread::detach() {
  assert(joinable());
  pthread_detach(thread_);
  joined_ = true;
}

void* Base::Thread::threadMainFunc(void* arg) {
  auto self = static_cast<Thread*>(arg);
  cacheTidAndTidStr();
  self->threadId_ = t_threadId;
  t_threadName = self->threadName_;
  {
    LockGuard guard(self->mutex_);
    self->started_ = true;
    self->condition_.notifyOne();
  }
  auto task = std::move(self->task_);
  if (task) task();
  return nullptr;
}