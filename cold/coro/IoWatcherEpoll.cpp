#include "cold/coro/IoWatcherEpoll.h"

#include <sys/eventfd.h>
#include <unistd.h>

#include <coroutine>

#include "cold/coro/IoWatcher.h"
#include "cold/log/Logger.h"
#include "cold/thread/Thread.h"
#include "cold/util/ScopeUtil.h"

using namespace Cold::Base;

using EpollEventEntry = IoWatcherEpoll::EpollEventEntry;

int CreateEventFd() {
  int eventFd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (eventFd < 0) {
    LOG_FATAL(GetMainLogger(), "Cannot create EventFd");
  };
  return eventFd;
}

EpollEventEntry EpollEventEntry::FromIoEvent(const internal::IoEvent& event) {
  IoWatcherEpoll::EpollEventEntry entry;
  assert(event.mode != internal::Mode::DISABLE_ALL &&
         event.mode != internal::Mode::DISABLE_READ &&
         event.mode != internal::Mode::DISABLE_WRITE);
  entry.fd = event.fd;
  entry.interest = static_cast<uint32_t>(event.mode);
  if (event.mode == internal::Mode::READ) {
    entry.readCallbackCoroutine = event.callbackCoroutine;
  } else {
    entry.writeCallbackCoroutine = event.callbackCoroutine;
  }
  return entry;
}

void EpollEventEntry::UpdateInterest(internal::Mode mode) {
  if (mode == internal::Mode::READ || mode == internal::Mode::WRITE)
    interest |= static_cast<uint32_t>(mode);
  else
    interest &= static_cast<uint32_t>(mode);
}

IoWatcherEpoll::IoWatcherEpoll()
    : epollFd_(epoll_create1(EPOLL_CLOEXEC)), wakeupFd_(CreateEventFd()) {
  if (epollFd_.Fd() < 0) {
    LOG_FATAL(GetMainLogger(), "Cannot create epollfd Reson:{}",
              ThisThread::ErrorMsg());
  }
  epollEvents_.resize(16);
  internal::IoEvent wakeupEvent;
  wakeupEvent.fd = wakeupFd_.Fd();
  wakeupEvent.mode = internal::Mode::READ;
  HandleIoEvent(wakeupEvent);
}

IoWatcherEpoll::~IoWatcherEpoll() {
  internal::IoEvent wakeupEvent;
  wakeupEvent.fd = wakeupFd_.Fd();
  wakeupEvent.mode = internal::Mode::DISABLE_ALL;
  // HandleIoEvent(wakeupEvent);
}

void IoWatcherEpoll::HandleIoEvent(const internal::IoEvent& event) {
  auto it = entries_.find(event.fd);
  if (it == entries_.end()) {
    if (event.mode != internal::Mode::READ &&
        event.mode != internal::Mode::WRITE) {
      LOG_ERROR(GetMainLogger(), "Bad IoEvent");
      return;
    }
    auto entry = EpollEventEntry::FromIoEvent(event);
    entries_[entry.fd] = entry;
    AddEpollEvent(entries_[entry.fd]);
  } else {
    auto& entry = it->second;
    entry.UpdateInterest(event.mode);
    UpdateEpollEvent(entry);
  }
}

void IoWatcherEpoll::AddEpollEvent(EpollEventEntry& entry) {
  struct epoll_event event;
  event.events = entry.interest;
  event.data.ptr = &entry;
  int ret = epoll_ctl(epollFd_.Fd(), EPOLL_CTL_ADD, entry.fd, &event);
  if (ret == 0) {
    entry.registerEvent = entry.interest;
  } else {
    LOG_ERROR(GetMainLogger(), "epoll_ctl add error fd = {},reason = {}",
              entry.fd, ThisThread::ErrorMsg());
    entries_.erase(entry.fd);
  }
}

void IoWatcherEpoll::UpdateEpollEvent(EpollEventEntry& entry) {
  struct epoll_event event;
  event.events = entry.interest;
  event.data.ptr = &entry;
  int operation = entry.interest ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
  int ret = epoll_ctl(epollFd_.Fd(), operation, entry.fd, &event);
  if (ret == 0) {
    if (operation == EPOLL_CTL_MOD)
      entry.registerEvent = entry.interest;
    else
      entries_.erase(entry.fd);
  } else {
    LOG_ERROR(GetMainLogger(), "epoll_ctl {} error fd = {},reason = {}",
              operation == EPOLL_CTL_MOD ? "mod" : "del", entry.fd,
              ThisThread::ErrorMsg());
  }
}

const std::vector<std::coroutine_handle<>>& IoWatcherEpoll::WatchIo(
    int waitTime) {
  activeCoroutines_.clear();
  const int epoll_result =
      epoll_wait(epollFd_.Fd(), epollEvents_.data(),
                 static_cast<int>(epollEvents_.size()), waitTime);
  if (epoll_result < 0) {
    LOG_ERROR(GetMainLogger(), "epoll_wait error reson:{}",
              ThisThread::ErrorMsg());
  } else {
    size_t size = static_cast<size_t>(epoll_result);
    LOG_TRACE(GetMainLogger(), "WaitIo total fd:{}", size);
    if (epollEvents_.size() == size) epollEvents_.resize(size << 1);
    for (size_t i = 0; i < size; ++i) {
      auto& epollEvent = epollEvents_[i];
      auto events = epollEvent.events;
      auto& entry = *static_cast<EpollEventEntry*>(epollEvent.data.ptr);
      if (entry.fd == wakeupFd_.Fd()) {
        HandleWakeup();
        continue;
      }
      const bool readable = (events & EPOLLIN) != 0;
      const bool writable = (events & EPOLLOUT) != 0;
      const bool disconnected = (events & (EPOLLHUP | EPOLLERR)) != 0;
      if (readable || disconnected) {
        entry.interest &= ~EPOLLIN;
        activeCoroutines_.push_back(entry.readCallbackCoroutine);
        entry.readCallbackCoroutine = std::noop_coroutine();
      }
      if (writable) {
        entry.interest &= ~EPOLLOUT;
        activeCoroutines_.push_back(entry.writeCallbackCoroutine);
        entry.readCallbackCoroutine = std::noop_coroutine();
      }
      LOG_TRACE(GetMainLogger(),
                "IoEvent Info fd:{} readable:{} writeable:{} disconnected:{}",
                entry.fd, readable, writable, disconnected);
      UpdateEpollEvent(entry);
    }
  }
  return activeCoroutines_;
}

void IoWatcherEpoll::Wakeup() {
  uint64_t value = 666;
  if (write(wakeupFd_.Fd(), &value, sizeof value) != sizeof value) {
    LOG_ERROR(GetMainLogger(), "Wakeup Error Reason:{}",
              ThisThread::ErrorMsg());
  }
}

void IoWatcherEpoll::HandleWakeup() {
  uint64_t value = 0;
  LOG_TRACE(GetMainLogger(), "HandleWakeup");
  if (read(wakeupFd_.Fd(), &value, sizeof value) != sizeof value) {
    LOG_ERROR(GetMainLogger(), "HandleWakeup Error Reason:{}",
              ThisThread::ErrorMsg());
  }
}