#ifndef COLD_IO_IOEVENT
#define COLD_IO_IOEVENT

#include <sys/epoll.h>

#include <coroutine>
#include <string>

namespace Cold {

class IoWatcher;
class IoContext;

class IoEvent {
 public:
  friend class IoWatcher;
  IoEvent() = default;
  ~IoEvent() = default;

  void SetOnReadCoroutine(std::coroutine_handle<> onRead) { onRead_ = onRead; }
  void SetOnWriteCoroutine(std::coroutine_handle<> onWrite) {
    onWrite_ = onWrite;
  }

  void EnableReading();
  void EnableReadingET();

  void EnableWriting();
  void EnableWritingET();

  void DisableReading();

  void DisableWriting();

  void DisableAll();

  int GetFd() const { return fd_; }
  uint32_t GetEvents() const { return events_; }
  uint32_t GetEventsInEpoll() const { return eventsInEpoll_; }

  static std::string DumpEv(uint32_t e) {
    std::string res;
    if (e & EPOLLIN) res += "EPOLLIN ";
    if (e & EPOLLOUT) res += "EPOLLOUT ";
    if (e & EPOLLET) res += "EPOLLET ";
    if (e & EPOLLHUP) res += "EPOLLHUP ";
    if (e & EPOLLPRI) res += "EPOLLPRI ";
    if (e & EPOLLERR) res += "EPOLLERR ";
    return res;
  }

  std::string DumpEvents() const { return DumpEv(events_); }
  std::string DumpEventsInEpoll() const { return DumpEv(eventsInEpoll_); }

  void ClearReadCoroutine() { onRead_ = std::coroutine_handle<>(); }
  void ClearWriteCoroutine() { onWrite_ = std::coroutine_handle<>(); }

  void ReturnIoEvent();

  IoContext& GetIoContext() const;

 private:
  int fd_ = -1;
  uint32_t events_ = 0;
  uint32_t eventsInEpoll_ = 0;
  IoWatcher* watcher_ = nullptr;
  std::coroutine_handle<> onRead_;
  std::coroutine_handle<> onWrite_;
};

}  // namespace Cold

#endif /* COLD_IO_IOEVENT */
