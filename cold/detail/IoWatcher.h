#ifndef COLD_DETAIL_IOWATCHER
#define COLD_DETAIL_IOWATCHER

#include <sys/eventfd.h>

#include <map>
#include <vector>

#include "../log/Log.h"
#include "IoEvent.h"

namespace Cold::Detail {

class IoWatcher {
 public:
  IoWatcher(IoContext* ioContext)
      : ioContext_(ioContext),
        epollFd_(epoll_create1(EPOLL_CLOEXEC)),
        wakeUpFd_(eventfd(0, EFD_CLOEXEC)) {
    epollEvents_.resize(64);
    if (epollFd_ < 0) {
      FATAL("create epoll fd error. reason: {}", ThisThread::ErrorMsg());
    }
    if (wakeUpFd_ < 0) {
      FATAL("create event fd error. reason: {}", ThisThread::ErrorMsg());
    }
    TakeIoEvent(wakeUpFd_)->EnableReading();
  }

  ~IoWatcher() {
    ioEvents_[wakeUpFd_]->ReturnIoEvent();
    close(wakeUpFd_);
    close(epollFd_);
  }

  void WatchIo(std::vector<std::coroutine_handle<>>& pending, int waitMs) {
    const int ioCnt = epoll_wait(epollFd_, epollEvents_.data(),
                                 static_cast<int>(epollEvents_.size()), waitMs);
    if (ioCnt < 0) {
      ERROR("epoll_wait error. reason: {}", ThisThread::ErrorMsg());
      return;
    }
    size_t size = static_cast<size_t>(ioCnt);
    TRACE("WatchIo total active fd: {} ioEventMap size:{}", size,
          ioEvents_.size());
    for (size_t i = 0; i < size; ++i) {
      auto& ev = epollEvents_[i];
      auto* ioEvent = static_cast<IoEvent*>(ev.data.ptr);
      assert(ioEvents_.count(ioEvent->fd_));
      assert(ioEvents_[ioEvent->fd_].get() == ioEvent);
      TRACE("Current solve fd: {}, happen events: {}, watch events: {}", size,
            IoEvent::DumpEv(ev.events), ioEvent->DumpEventsInEpoll());
      if (ioEvent->fd_ == wakeUpFd_) {
        HandleWakeUp();
        continue;
      }
      const bool readable = (ev.events & EPOLLIN) != 0;
      const bool writable = (ev.events & EPOLLOUT) != 0;
      const bool disconnected = (ev.events & (EPOLLHUP | EPOLLERR)) != 0;
      if ((readable || disconnected)) {  // triggle read
        if (ioEvent->onRead_) {
          pending.push_back(ioEvent->onRead_);
        }
        ioEvent->onRead_ = std::coroutine_handle<>();
      }
      if (writable) {  // triggle write
        if (ioEvent->onWrite_) {
          pending.push_back(ioEvent->onWrite_);
        }
        ioEvent->onWrite_ = std::coroutine_handle<>();
      }
    }
    if (epollEvents_.size() == size) epollEvents_.resize(size << 1);
  }

  std::shared_ptr<IoEvent> TakeIoEvent(int fd) {
    assert(!ioEvents_.count(fd));
    auto ev = std::make_shared<IoEvent>();
    ev->fd_ = fd;
    ev->watcher_ = this;
    ioEvents_[fd] = ev;
    return ev;
  }

  void UpdateIoEvent(IoEvent* ioEvent) {
    assert(ioEvents_.count(ioEvent->fd_));
    if (ioEvent->eventsInEpoll_ == 0 && ioEvent->events_ == 0) {
      return;
    }
    if (ioEvent->eventsInEpoll_ == ioEvent->events_) {
      ERROR("update io event with same events. fd: {} ev: {}", ioEvent->fd_,
            ioEvent->DumpEvents());
      return;
    }
    struct epoll_event ev;
    ev.events = ioEvent->events_;
    ev.data.ptr = ioEvent;
    int op = EPOLL_CTL_MOD;
    if (ioEvent->eventsInEpoll_ == 0) {
      op = EPOLL_CTL_ADD;
    } else if (ioEvent->events_ == 0) {
      op = EPOLL_CTL_DEL;
    }
    if (epoll_ctl(epollFd_, op, ioEvent->fd_, &ev) < 0) {
      ERROR("epoll_ctl error. error fd: {}, reason: {}", ioEvent->fd_,
            ThisThread::ErrorMsg());
      return;
    }
    ioEvent->eventsInEpoll_ = ioEvent->events_;
  }

  void ReturnIoEvent(IoEvent* ev) {
    assert(ioEvents_.count(ev->fd_));
    assert(ioEvents_[ev->fd_].get() == ev);
    ev->DisableAll();
    ioEvents_.erase(ev->fd_);
  }

  IoContext& GetIoContext() const { return *ioContext_; }

  void WakeUp() {
    uint64_t one = 1;
    if (write(wakeUpFd_, &one, sizeof(one)) < 0) {
      ERROR("wake up error. reason: {}", ThisThread::ErrorMsg());
    }
  }

 private:
  void HandleWakeUp() {
    uint64_t one = 1;
    if (read(wakeUpFd_, &one, sizeof(one)) < 0) {
      ERROR("handle wake up error. reason: {}", ThisThread::ErrorMsg());
    }
  }

  IoContext* ioContext_;
  int epollFd_;
  int wakeUpFd_;
  std::vector<struct epoll_event> epollEvents_;
  std::map<int, std::shared_ptr<IoEvent>> ioEvents_;
};

inline void IoEvent::UpdateEvent() {
  assert(watcher_);
  watcher_->UpdateIoEvent(this);
}

inline void IoEvent::ReturnIoEvent() {
  assert(watcher_);
  watcher_->ReturnIoEvent(this);
}

inline IoContext& IoEvent::GetIoContext() const {
  assert(watcher_);
  return watcher_->GetIoContext();
}

}  // namespace Cold::Detail

#endif /* COLD_DETAIL_IOWATCHER */
