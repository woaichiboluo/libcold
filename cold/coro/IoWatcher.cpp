#include "cold/coro/IoWatcher.h"

#include <sys/eventfd.h>
#include <unistd.h>

#include <cassert>
#include <coroutine>

#include "cold/log/Logger.h"

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
  auto& event = ioEvents_[wakeUpFd_];
  event.fd = wakeUpFd_;
  event.events = EPOLLIN;
  struct epoll_event ev;
  ev.events = event.events;
  ev.data.ptr = &ioEvents_[wakeUpFd_];
  if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, wakeUpFd_, &ev) < 0) {
    Base::FATAL("epoll_ctl error. error fd: {}, reason: {}", wakeUpFd_,
                ThisThread::ErrorMsg());
  }
  epollEvents_.resize(16);
}

Base::IoWatcher::~IoWatcher() {
  epoll_ctl(epollFd_, EPOLL_CTL_DEL, wakeUpFd_, nullptr);
  close(wakeUpFd_);
  close(epollFd_);
}

void Base::IoWatcher::ListenReadEvent(int fd, Handle handle) {
  if (!ioEvents_.count(fd)) {
    IoEvent& event = ioEvents_[fd];
    event.events = EPOLLIN | EPOLLET;
    event.fd = fd;
    event.readHandle = handle;
    struct epoll_event ev;
    ev.events = event.events;
    ev.data.ptr = &ioEvents_[fd];
    if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, event.fd, &ev) < 0) {
      Base::FATAL("epoll_ctl error. error fd: {}, reason: {}", event.fd,
                  ThisThread::ErrorMsg());
    }
  } else {
    auto& event = ioEvents_[fd];
    assert(event.readHandle == std::noop_coroutine());
    event.readHandle = handle;
  }
}

void Base::IoWatcher::ListenWriteEvent(int fd, Handle handle) {
  int op = EPOLL_CTL_MOD;
  if (!ioEvents_.count(fd)) op = EPOLL_CTL_ADD;
  auto& event = ioEvents_[fd];
  assert(event.writeHandle == std::noop_coroutine());
  event.fd = fd;
  event.writeHandle = handle;
  auto old = event.events;
  event.events |= EPOLLOUT | EPOLLET;
  struct epoll_event ev;
  ev.events = event.events;
  ev.data.ptr = &ioEvents_[fd];
  if (epoll_ctl(epollFd_, op, event.fd, &ev) < 0) {
    Base::ERROR("epoll_ctl error. error fd: {}, reason: {}", event.fd,
                ThisThread::ErrorMsg());
    event.events = old;
  }
}

void Base::IoWatcher::StopListeningReadEvent(int fd) {
  assert(ioEvents_.count(fd));
  auto& event = ioEvents_[fd];
  if (event.readHandle == std::noop_coroutine()) return;
  event.readHandle = std::noop_coroutine();
}

void Base::IoWatcher::StopListeningWriteEvent(int fd) {
  int op = EPOLL_CTL_MOD;
  assert(ioEvents_.count(fd));
  auto& event = ioEvents_[fd];
  event.writeHandle = std::noop_coroutine();
  event.events &= ~EPOLLOUT;
  if (!(event.events & EPOLLIN)) op = EPOLL_CTL_DEL;
  struct epoll_event ev;
  ev.events = event.events;
  ev.data.ptr = &ioEvents_[fd];
  if (epoll_ctl(epollFd_, op, event.fd, &ev) < 0) {
    Base::ERROR("epoll_ctl error. error fd: {}, reason: {}", event.fd,
                ThisThread::ErrorMsg());
  }
  if (op == EPOLL_CTL_DEL) ioEvents_.erase(fd);
}

void Base::IoWatcher::StopListeningAll(int fd) {
  if (!ioEvents_.count(fd)) return;
  auto& event = ioEvents_[fd];
  struct epoll_event ev;
  event.events = 0;
  ev.events = event.events;
  ev.data.ptr = &ioEvents_[fd];
  if (epoll_ctl(epollFd_, EPOLL_CTL_DEL, event.fd, &ev) < 0) {
    Base::ERROR("epoll_ctl error. error fd: {}, reason: {}", event.fd,
                ThisThread::ErrorMsg());
  }
  ioEvents_.erase(fd);
}

void Base::IoWatcher::WakeUp() {
  uint64_t value = 666;
  if (write(wakeUpFd_, &value, sizeof value) != sizeof value) {
    Base::ERROR("WakeUp Error reason: {}", ThisThread::ErrorMsg());
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
      IoEvent& event = *static_cast<IoEvent*>(epollEvent.data.ptr);
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
      Base::DEBUG(
          "IoEvent Info fd: {}, readable: {}, writeable: {}, disconnected: {}",
          event.fd, readable, writable, disconnected);
      // use et always read
      if ((readable || disconnected) &&
          (event.readHandle != std::noop_coroutine())) {
        activeCoroutines_.push_back(event.readHandle);
        event.readHandle = std::noop_coroutine();
      }
      if (writable) {
        event.events &= ~EPOLLOUT;
        activeCoroutines_.push_back(event.writeHandle);
        StopListeningWriteEvent(event.fd);
      }
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