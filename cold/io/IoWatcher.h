#ifndef COLD_IO_IOWATCHER
#define COLD_IO_IOWATCHER

#include <map>
#include <vector>

#include "IoEvent.h"

namespace Cold {

class IoWatcher {
 public:
  IoWatcher();
  ~IoWatcher();

  void WatchIo(std::vector<std::coroutine_handle<>>& pending, int waitMs);

  void UpdateIoEvent(IoEvent* ioEvent);

  void WakeUp();

  IoEvent* TakeIoEvent(int fd);
  void ReturnIoEvent(IoEvent* ev);

 private:
  void HandleWakeUp();

  int epollFd_;
  int wakeUpFd_;
  std::vector<struct epoll_event> epollEvents_;
  std::map<int, IoEvent> ioEvents_;
};

}  // namespace Cold

#endif /* COLD_IO_IOWATCHER */
