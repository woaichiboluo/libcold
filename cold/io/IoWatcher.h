#ifndef COLD_IO_IOWATCHER
#define COLD_IO_IOWATCHER

#include <map>
#include <vector>

#include "IoEvent.h"

namespace Cold {

class IoWatcher {
 public:
  IoWatcher(IoContext* ioContext);
  ~IoWatcher();

  void WatchIo(std::vector<std::coroutine_handle<>>& pending, int waitMs);

  void UpdateIoEvent(IoEvent* ioEvent);

  void WakeUp();

  IoEvent* TakeIoEvent(int fd);
  void ReturnIoEvent(IoEvent* ev);

  IoContext& GetIoContext() const { return *ioContext_; }

 private:
  void HandleWakeUp();

  IoContext* ioContext_;
  int epollFd_;
  int wakeUpFd_;
  std::vector<struct epoll_event> epollEvents_;
  std::map<int, IoEvent> ioEvents_;
};

}  // namespace Cold

#endif /* COLD_IO_IOWATCHER */
