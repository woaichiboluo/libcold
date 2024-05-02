#include "cold/coro/IoWatcher.h"

#include <sys/eventfd.h>
#include <unistd.h>

#include <cassert>
#include <coroutine>

#include "cold/log/Logger.h"
#include "cold/thread/Lock.h"

using namespace Cold;

Base::IoWatcher::IoWatcher()
    : epollFd_(epoll_create1(EPOLL_CLOEXEC)),
      wakeUpFd_(eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK)) {
  if (epollFd_ < 0) {
    Base::FATAL("Create epoll fd error. reason: {}", ThisThread::ErrorMsg());
  }
  if (wakeUpFd_ < 0) {
    Base::FATAL("Create event fd error. reason: {}", ThisThread::ErrorMsg());
  }
  IoEvent event;
  event.fd = wakeUpFd_;
  event.events = EPOLLIN;
  HandleIoEvent(event, EPOLL_CTL_ADD);
  epollEvents_.resize(16);
}

Base::IoWatcher::~IoWatcher() {
  epoll_ctl(epollFd_, EPOLL_CTL_DEL, wakeUpFd_, nullptr);
  close(wakeUpFd_);
  close(epollFd_);
}

void Base::IoWatcher::ListenReadEvent(int fd, Handle handle) {
  ListenEvent(fd, handle, EPOLLIN);
}

void Base::IoWatcher::ListenWriteEvent(int fd, Handle handle) {
  ListenEvent(fd, handle, EPOLLOUT);
}

void Base::IoWatcher::StopListeningReadEvent(int fd) {
  StopListeningEvent(fd, EPOLLIN);
}

void Base::IoWatcher::StopListeningWriteEvent(int fd) {
  StopListeningEvent(fd, EPOLLOUT);
}

void Base::IoWatcher::StopListeningAll(int fd) {
  {
    LockGuard guard(mutexForIoEvents_);
    if (!ioEvents_.count(fd)) return;
  }
  StopListeningEvent(fd, EPOLLIN | EPOLLOUT);
}

void Base::IoWatcher::WakeUp() {
  uint64_t value = 666;
  if (write(wakeUpFd_, &value, sizeof value) != sizeof value) {
    Base::ERROR("WakeUp Error reason: {}", ThisThread::ErrorMsg());
  }
}

void Base::IoWatcher::ListenEvent(int fd, Handle handle, uint32_t ev) {
  assert(ev == EPOLLIN || ev == EPOLLOUT);
  IoEvent event;
  int operation = EPOLL_CTL_ADD;
  event.fd = fd;
  event.events = ev;
  {
    LockGuard guard(mutexForIoEvents_);
    auto it = ioEvents_.find(fd);
    if (it != ioEvents_.end()) {
      event = it->second;
      assert(event.fd == fd);
      assert(event.events != 0);
      assert(event.events != ev);
      operation = EPOLL_CTL_MOD;
      event.events |= ev;
    }
  }
  if (ev == EPOLLIN) {
    event.readHandle = handle;
  } else {
    event.writeHandle = handle;
  }
  HandleIoEvent(event, operation);
}

void Base::IoWatcher::StopListeningEvent(int fd, uint32_t disableEvent) {
  IoEvent event;
  {
    LockGuard guard(mutexForIoEvents_);
    auto it = ioEvents_.find(fd);
    assert(it != ioEvents_.end());
    event = it->second;
  }
  assert(event.events != 0);
  event.events &= ~disableEvent;
  if (disableEvent & EPOLLIN) {
    event.readHandle = std::noop_coroutine();
  }
  if (disableEvent & EPOLLOUT) {
    event.writeHandle = std::noop_coroutine();
  }
  int operation = event.events == 0 ? EPOLL_CTL_DEL : EPOLL_CTL_MOD;
  HandleIoEvent(event, operation);
}

void Base::IoWatcher::HandleIoEvent(const IoEvent& event, int operation) {
  LockGuard guard(mutexForIoEvents_);
  struct epoll_event ev;
  ev.events = event.events;
  IoEvent previous;
  if (operation != EPOLL_CTL_ADD) {
    assert(ioEvents_.count(event.fd));
    previous = ioEvents_[event.fd];
  }
  ioEvents_[event.fd] = event;
  ev.data.ptr = &ioEvents_[event.fd];
  if (epoll_ctl(epollFd_, operation, event.fd, &ev) != 0) {
    Base::ERROR("epool_ctl error. error fd: {}, reason: {}", event.fd,
                ThisThread::ErrorMsg());
    ioEvents_[event.fd] = previous;
    if (operation == EPOLL_CTL_ADD) {
      ioEvents_.erase(event.fd);
    }
    return;
  }
  if (operation == EPOLL_CTL_DEL) {
    ioEvents_.erase(event.fd);
  }
}

std::string DumpEpollEvent(uint32_t ev) {
  std::string res;
  if (ev & EPOLLIN) res += "EPOLLIN ";
  if (ev & EPOLLOUT) res += "EPOLLOUT ";
  if (ev & EPOLLHUP) res += "EPOLLHUP ";
  if (ev & EPOLLPRI) res += "EPOLLPRI ";
  if (ev & EPOLLERR) res += "EPOLLERR ";
  return res;
}

const std::vector<std::coroutine_handle<>>& Base::IoWatcher::WatchIo(
    int waitMs) {
  activeCoroutines_.clear();
  const int epoll_result =
      epoll_wait(epollFd_, epollEvents_.data(),
                 static_cast<int>(epollEvents_.size()), waitMs);
  if (epoll_result < 0) {
    Base::ERROR("epoll_wait error reson: {}", ThisThread::ErrorMsg());
  } else {
    size_t size = static_cast<size_t>(epoll_result);
    Base::TRACE("WatchIo total active fd: {}", size);
    for (size_t i = 0; i < size; ++i) {
      auto& epollEvent = epollEvents_[i];
      auto events = epollEvent.events;
      IoEvent event;
      {
        LockGuard guard(mutexForIoEvents_);
        event = *static_cast<IoEvent*>(epollEvent.data.ptr);
      }
      if (event.fd == wakeUpFd_) {
        HandleWakeUp();
        continue;
      }
      Base::DEBUG(
          "In WaitIo Current solve fd: {} ,in epoll events: {}, in ioEvents "
          "events: {} ",
          event.fd, DumpEpollEvent(events), DumpEpollEvent(event.events));
      const bool readable = (events & EPOLLIN) != 0;
      const bool writable = (events & EPOLLOUT) != 0;
      const bool disconnected = (events & (EPOLLHUP | EPOLLERR)) != 0;
      if (readable || disconnected) {
        event.events &= ~EPOLLIN;
        activeCoroutines_.push_back(event.readHandle);
        event.readHandle = std::noop_coroutine();
      }
      if (writable) {
        event.events &= ~EPOLLOUT;
        activeCoroutines_.push_back(event.writeHandle);
        event.writeHandle = std::noop_coroutine();
      }
      Base::DEBUG(
          "IoEvent Info fd: {}, readable: {}, writeable: {}, disconnected: {}",
          event.fd, readable, writable, disconnected);
      HandleIoEvent(event, event.events == 0 ? EPOLL_CTL_DEL : EPOLL_CTL_MOD);
    }
    if (epollEvents_.size() == size) epollEvents_.resize(size << 1);
  }
  return activeCoroutines_;
}

void Base::IoWatcher::HandleWakeUp() {
  uint64_t value = 0;
  Base::TRACE("HandleWakeUp");
  if (read(wakeUpFd_, &value, sizeof value) != sizeof value) {
    Base::ERROR("HandleWakeUp Error Reason:{}", ThisThread::ErrorMsg());
  }
}

std::string Base::IoWatcher::IoEvent::Dump() {
  return fmt::format("IoEvent(fd: {}, Read: {}, Write: {})", fd,
                     events & EPOLLIN, events & EPOLLOUT);
}