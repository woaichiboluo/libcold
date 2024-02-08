#ifndef COLD_CORO_IOWATCHEREPOLL
#define COLD_CORO_IOWATCHEREPOLL

#include <sys/epoll.h>

#include <coroutine>
#include <map>

#include "cold/coro/IoWatcher.h"
#include "cold/util/ScopeUtil.h"

namespace Cold::Base {

class IoWatcherEpoll : public IoWatcher {
 public:
  struct EpollEventEntry {
    int fd = -1;
    uint32_t interest = 0;
    uint32_t registerEvent = 0;
    std::coroutine_handle<> readCallbackCoroutine = std::noop_coroutine();
    std::coroutine_handle<> writeCallbackCoroutine = std::noop_coroutine();

    static EpollEventEntry FromIoEvent(const internal::IoEvent& event);
    void UpdateEntry(internal::Mode mode);
  };

  IoWatcherEpoll();
  ~IoWatcherEpoll() override;

  const std::vector<std::coroutine_handle<>>& WatchIo(int waitTime) override;
  void HandleIoEvent(const internal::IoEvent& event) override;
  void Wakeup() override;

 private:
  void AddEpollEvent(EpollEventEntry& entry);
  void UpdateEpollEvent(EpollEventEntry& entry);
  void HandleWakeup();

  ScopeFdGuard epollFd_;
  ScopeFdGuard wakeupFd_;
  std::map<int, EpollEventEntry> entries_;
  std::vector<struct epoll_event> epollEvents_;
  std::vector<std::coroutine_handle<>> activeCoroutines_;
};

};  // namespace Cold::Base

#endif /* COLD_CORO_IOWATCHEREPOLL */
