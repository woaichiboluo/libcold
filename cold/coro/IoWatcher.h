#ifndef COLD_CORO_IOWATCHER
#define COLD_CORO_IOWATCHER

#include <sys/epoll.h>

#include <coroutine>
#include <map>
#include <string>
#include <vector>

namespace Cold::Base {

class IoWatcher {
  friend class IoService;

 public:
  using Handle = std::coroutine_handle<>;
  IoWatcher();
  ~IoWatcher();

  IoWatcher(const IoWatcher&) = delete;
  IoWatcher& operator=(const IoWatcher&) = delete;

  void ListenReadEvent(int fd, Handle handle);
  void ListenWriteEvent(int fd, Handle handle);
  void StopListeningReadEvent(int fd);
  void StopListeningWriteEvent(int fd);
  void StopListeningAll(int fd);

 private:
  struct IoEvent {
    int fd = 0;
    uint32_t events = 0;
    Handle readHandle = std::noop_coroutine();
    Handle writeHandle = std::noop_coroutine();
    std::string Dump();
  };

  const std::vector<Handle>& WatchIo(int waitMs);

  void WakeUp();

  void ListenEvent(int fd, Handle handle, uint32_t ev);
  void StopListeningEvent(int fd, uint32_t ev);

  void HandleIoEvent(const IoEvent& event, int operation);
  void HandleWakeUp();

  int epollFd_;
  int wakeUpFd_;
  std::vector<struct epoll_event> epollEvents_;
  std::map<int, IoEvent> ioEvents_;
  std::vector<Handle> activeCoroutines_;
};

}  // namespace Cold::Base

#endif /* COLD_CORO_IOWATCHER */
