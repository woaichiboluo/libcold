#ifndef COLD_NET_IOAWAITABLE
#define COLD_NET_IOAWAITABLE

#include <sys/socket.h>
#include <unistd.h>

#include <chrono>

#include "cold/coro/IoContext.h"
#include "cold/coro/Timer.h"
#include "cold/net/IpAddress.h"

namespace Cold::Net {

class ReadAwaitable {
 public:
  ReadAwaitable(Base::IoContext* ioContext, int fd, void* buf, size_t count,
                bool avail)
      : ioContext_(ioContext),
        fd_(fd),
        buf_(buf),
        count_(count),
        readAvailable_(avail) {}

  bool await_ready() noexcept {
    if (!readAvailable_) return true;
    return false;
  }

  void await_suspend(std::coroutine_handle<> handle) noexcept {
    assert(readAvailable_);
    ioContext_->AddReadIoEvent(fd_, handle);
  }

  ssize_t await_resume() noexcept {
    if (!readAvailable_ || timeout_) {
      errno = timeout_ ? ETIMEDOUT : ENOTCONN;
      return -1;
    }
    auto n = read(fd_, buf_, count_);
    return n;
  }

  void SetTimeout() {
    timeout_ = true;
    ioContext_->RemoveReadIoEvent(fd_);
  }
  bool GetTimeout() const { return timeout_; }

 private:
  Base::IoContext* ioContext_;
  int fd_;
  void* buf_;
  size_t count_;
  bool readAvailable_;
  bool timeout_ = false;
};

class WriteAwaitable {
 public:
  WriteAwaitable(Base::IoContext* ioContext, int fd, const void* buf,
                 size_t count, bool avail)
      : ioContext_(ioContext),
        fd_(fd),
        buf_(buf),
        count_(count),
        writeAvailable_(avail) {}

  bool await_ready() noexcept {
    if (!writeAvailable_) return true;
    auto pre = errno;
    retValue_ = write(fd_, buf_, count_);
    if (retValue_ >= 0) return true;
    if (errno == EAGAIN) {
      errno = pre;
      return false;
    }
    errorOccurred_ = true;
    return true;
  }

  void await_suspend(std::coroutine_handle<> handle) noexcept {
    assert(writeAvailable_);
    assert(!errorOccurred_);
    ioContext_->AddWriteIoEvent(fd_, handle);
  }

  ssize_t await_resume() noexcept {
    if (!writeAvailable_ || timeout_) {
      errno = timeout_ ? ETIMEDOUT : ENOTCONN;
      return -1;
    }
    if (errorOccurred_ || retValue_ >= 0) return retValue_;
    return write(fd_, buf_, count_);
  }

  void SetTimeout() {
    timeout_ = true;
    ioContext_->RemoveWriteIoEvent(fd_);
  }
  bool GetTimeout() const { return timeout_; }

 private:
  Base::IoContext* ioContext_;
  int fd_;
  const void* buf_;
  size_t count_;
  bool writeAvailable_;
  bool errorOccurred_ = false;
  ssize_t retValue_ = -1;
  bool timeout_ = false;
};

class AcceptAwaitable {
 public:
  AcceptAwaitable(Base::IoContext* ioContext, int fd)
      : ioContext_(ioContext), fd_(fd) {}

  bool await_ready() noexcept { return false; }

  void await_suspend(std::coroutine_handle<> handle) noexcept {
    ioContext_->AddReadIoEvent(fd_, handle);
  }

  std::pair<int, IpAddress> await_resume() noexcept {
    socklen_t arrlen = sizeof(addr_);
    auto peer = accept4(fd_, reinterpret_cast<struct sockaddr*>(&addr_),
                        &arrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
    return {peer, IpAddress(addr_)};
  }

 private:
  Base::IoContext* ioContext_;
  int fd_;
  struct sockaddr_in6 addr_;
};

template <typename AWAITABLE, typename PERIOD, typename REP>
class IoTimeoutAwaitable {
 public:
  using Duration = std::chrono::duration<PERIOD, REP>;
  using RetType =
      std::invoke_result_t<decltype(&AWAITABLE::await_resume), AWAITABLE>;

  IoTimeoutAwaitable(Base::IoContext* ioContext, AWAITABLE&& awaitable,
                     Duration timeoutTime)
      : ioContext_(ioContext),
        awaitable_(std::move(awaitable)),
        timeoutTime_(timeoutTime),
        timer_(*ioContext) {}

  bool await_ready() noexcept { return false; }

  void await_suspend(std::coroutine_handle<> handle) noexcept {
    auto task = [](std::coroutine_handle<> coro, RetType& retValue,
                   AWAITABLE& awaitable) -> Base::Task<> {
      retValue = co_await awaitable;
      coro.resume();
    }(handle, retValue_, awaitable_);
    timer_.ExpiresAfter(timeoutTime_);
    timer_.AsyncWait(
        [](std::coroutine_handle<> coro, AWAITABLE& awaitable) -> Base::Task<> {
          awaitable.SetTimeout();
          coro.resume();
          co_return;
        }(task.GetHandle(), awaitable_));
    ioContext_->CoSpawn(std::move(task));
  }

  RetType await_resume() noexcept {
    if (!awaitable_.GetTimeout()) timer_.Cancel();
    return retValue_;
  }

 private:
  Base::IoContext* ioContext_;
  AWAITABLE awaitable_;
  Duration timeoutTime_;
  Base::SteadyTimer timer_;
  RetType retValue_;
};

}  // namespace Cold::Net

#endif /* COLD_NET_IOAWAITABLE */
