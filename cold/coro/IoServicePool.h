#ifndef COLD_CORO_IOSERVICEPOOL
#define COLD_CORO_IOSERVICEPOOL

#include "cold/coro/IoService.h"
#include "cold/thread/Thread.h"

namespace Cold::Base {

class IoServicePool {
 public:
  IoServicePool(size_t poolSize = 0, std::string nameArg = "");
  ~IoServicePool();

  IoServicePool(const IoServicePool&) = delete;
  IoServicePool& operator=(const IoServicePool&) = delete;

  IoService& GetMainIoService();
  IoService& GetNextIoService();
  IoService& GetIoServiceForHash(size_t hashCode);

  size_t GetPoolSize() const { return poolSize_; }

  void Start();

 private:
  size_t poolSize_;
  std::string nameArg_;
  IoService mainIoService_;
  std::vector<std::unique_ptr<Thread>> threads_;
  std::vector<std::unique_ptr<IoService>> serviceVector_;
  std::atomic<size_t> index_;
};

}  // namespace Cold::Base

#endif /* COLD_CORO_IOSERVICEPOOL */
