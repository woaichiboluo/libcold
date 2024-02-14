#include "cold/coro/IoContextPool.h"

#include "cold/coro/IoContext.h"
#include "cold/log/Logger.h"

using namespace Cold;

Base::IoContextPool::IoContextPool(size_t threadNum) : threadNum_(threadNum) {
  for (size_t i = 0; i < threadNum + 1; ++i) {
    auto ioContext = std::make_unique<IoContext>();
    if (i != 0) {
      auto thread = std::make_unique<Thread>(
          [context = ioContext.get()]() { context->Start(); });
      threads_.push_back(std::move(thread));
    }
    contextVector_.push_back(std::move(ioContext));
  }
}

Base::IoContextPool::~IoContextPool() {
  for (auto& context : contextVector_) {
    context->Stop();
  }
  for (auto& thread : threads_) {
    if (thread->Started()) thread->Join();
  }
}

void Base::IoContextPool::Start() {
  for (auto& thread : threads_) {
    thread->Start();
  }
  contextVector_[0]->Start();
}

Base::IoContext* Base::IoContextPool::GetNextIoContext() {
  if (index_ >= contextVector_.size()) index_ = 0;
  return contextVector_[index_++].get();
}