#ifndef COLD_CORO_IOCONTEXTPOOL
#define COLD_CORO_IOCONTEXTPOOL

#include <atomic>
#include <memory>
#include <vector>

namespace Cold::Base {

class Thread;
class IoContext;

class IoContextPool {
 public:
  IoContextPool(size_t threadNum = 0);
  ~IoContextPool();

  IoContextPool(const IoContextPool&) = delete;
  IoContextPool& operator=(const IoContextPool&) = delete;

  IoContext* GetNextIoContext();
  void Start();

 private:
  size_t threadNum_;
  std::vector<std::unique_ptr<Thread>> threads_;
  std::vector<std::unique_ptr<IoContext>> contextVector_;
  std::atomic<size_t> index_ = 0;
};

}  // namespace Cold::Base

#endif /* COLD_CORO_IOCONTEXTPOOL */
