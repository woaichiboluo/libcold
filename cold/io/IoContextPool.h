#ifndef COLD_IO_IOCONTEXTPOOL
#define COLD_IO_IOCONTEXTPOOL

#include "IoContext.h"

namespace Cold {

class IoContextPool {
 public:
  IoContextPool(size_t poolSize, std::string nameArg = "pool")
      : poolSize_(poolSize), nameArg_(nameArg) {
    for (size_t i = 0; i < poolSize_; ++i) {
      contexts_.push_back(std::make_unique<IoContext>());
    }
  }

  ~IoContextPool() {
    if (started_) Stop();
  }

  void Start() {
    assert(!started_);
    for (size_t i = 0; i < poolSize_; ++i) {
      auto thread =
          Thread([context = contexts_[i].get()]() { context->Start(); },
                 nameArg_ + "-" + std::to_string(i));
      thread.Start();
      threads_.push_back(std::move(thread));
    }
    started_ = true;
    mainContext_.Start();
  }

  void Stop() {
    assert(started_);
    for (auto& context : contexts_) {
      context->Stop();
    }
    for (auto& thread : threads_) {
      thread.Join();
    }
    started_ = false;
    threads_.clear();
    mainContext_.Stop();
  }

  size_t GetPoolSize() const { return poolSize_; }
  bool IsStarted() const { return started_; }

  IoContext& GetMainIoContext() { return mainContext_; }

  IoContext& GetNextIoContext() {
    if (poolSize_ == 0) return mainContext_;
    auto context = contexts_[index_].get();
    index_ = (index_ + 1) % poolSize_;
    return *context;
  }

 private:
  size_t poolSize_;
  std::string nameArg_;
  IoContext mainContext_;
  std::atomic<bool> started_;
  size_t index_ = 0;
  std::vector<std::unique_ptr<IoContext>> contexts_;
  std::vector<Thread> threads_;
};

}  // namespace Cold

#endif /* COLD_IO_IOCONTEXTPOOL */
