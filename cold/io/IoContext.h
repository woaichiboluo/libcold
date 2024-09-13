#ifndef COLD_IO_IOCONTEXT
#define COLD_IO_IOCONTEXT

#include <atomic>
#include <memory>

#include "../coroutines/Scheduler.h"

namespace Cold {

class Scheduler;
class IoEvent;
class IoWatcher;

class IoContext {
 public:
  IoContext();
  ~IoContext() = default;

  void Start();
  void Stop();

  bool IsRunning() const { return running_; }

  IoContext(const IoContext&) = delete;
  IoContext& operator=(const IoContext&) = delete;

  void CoSpawn(Task<> task);

  IoEvent* TakeIoEvent(int fd);

 private:
  std::atomic<bool> running_ = false;
  std::unique_ptr<Scheduler> scheduler_;
  std::unique_ptr<IoWatcher> ioWatcher_;

  std::mutex mutexForPendingTasks_;
  std::vector<Task<>> pendingTasks_;
};

}  // namespace Cold

#endif /* COLD_IO_IOCONTEXT */
