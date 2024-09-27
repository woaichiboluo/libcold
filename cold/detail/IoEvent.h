#ifndef COLD_DETAIL_IOEVENT
#define COLD_DETAIL_IOEVENT

#include <sys/epoll.h>
#include <unistd.h>

#include <coroutine>
#include <string>

namespace Cold {
class IoContext;
}

namespace Cold::Detail {

class IoWatcher;

class IoEvent {
 public:
  using Handle = std::coroutine_handle<>;
  friend class IoWatcher;

  IoEvent() = default;

  ~IoEvent() = default;

  void SetOnReadCoroutine(Handle onRead) { onRead_ = onRead; }
  void SetOnWriteCoroutine(Handle onWrite) { onWrite_ = onWrite; }

  void EnableReading(bool et = false) {
    events_ |= et ? EPOLLIN | EPOLLET : EPOLLIN;
    UpdateEvent();
  }

  void EnableWriting(bool et = false) {
    events_ |= et ? EPOLLOUT | EPOLLET : EPOLLOUT;
    UpdateEvent();
  }

  void DisableReading() {
    events_ &= ~(EPOLLIN | EPOLLET);
    UpdateEvent();
  }

  void DisableWriting() {
    events_ &= ~(EPOLLOUT | EPOLLET);
    UpdateEvent();
  }

  void DisableAll() {
    events_ = 0;
    UpdateEvent();
  }

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

  void UpdateEvent();
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

}  // namespace Cold::Detail

#endif /* COLD_DETAIL_IOEVENT */
