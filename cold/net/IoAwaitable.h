#ifndef COLD_NET_IOAWAITABLE
#define COLD_NET_IOAWAITABLE

#include <sys/socket.h>
#include <unistd.h>

#include <chrono>

#include "cold/coro/IoContext.h"
#include "cold/coro/Timer.h"
#include "cold/net/IpAddress.h"

namespace Cold::Net {

class ReadAwaitable : public Base::AwaitableNonCopyable {
 public:
  ReadAwaitable(Base::IoContext* ioContext, int fd, void* buf, size_t count,
                bool connected)
      : ioContext_(ioContext),
        fd_(fd),
        buf_(buf),
        count_(count),
        connected_(connected) {}

  bool await_ready() noexcept {
    if (!connected_) return true;
    return false;
  }

  void await_suspend(std::coroutine_handle<> handle) noexcept {
    assert(connected_);
    ioContext_->AddReadIoEvent(fd_, handle);
  }

  ssize_t await_resume() noexcept {
    if (!connected_ || timeout_) {
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
  bool connected_;
  bool timeout_ = false;
};

class WriteAwaitable : public Base::AwaitableNonCopyable {
 public:
  WriteAwaitable(Base::IoContext* ioContext, int fd, const void* buf,
                 size_t count, bool connected)
      : ioContext_(ioContext),
        fd_(fd),
        buf_(buf),
        count_(count),
        connected_(connected) {}

  bool await_ready() noexcept {
    if (!connected_) return true;
    retValue_ = write(fd_, buf_, count_);
    if (retValue_ >= 0 || errno != EAGAIN) ready_ = true;
    return ready_;
  }

  void await_suspend(std::coroutine_handle<> handle) noexcept {
    assert(connected_);
    assert(!ready_);
    ioContext_->AddWriteIoEvent(fd_, handle);
  }

  ssize_t await_resume() noexcept {
    if (!connected_ || timeout_) {
      errno = timeout_ ? ETIMEDOUT : ENOTCONN;
      return -1;
    }
    if (ready_) return retValue_;
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
  bool connected_;
  bool ready_ = false;
  ssize_t retValue_ = -1;
  bool timeout_ = false;
};

class AcceptAwaitable : public Base::AwaitableNonCopyable {
 public:
  AcceptAwaitable(Base::IoContext* ioContext, int fd)
      : ioContext_(ioContext), fd_(fd) {}

  bool await_ready() noexcept { return false; }

  void await_suspend(std::coroutine_handle<> handle) noexcept {
    ioContext_->AddReadIoEvent(fd_, handle);
  }

  std::pair<int, IpAddress> await_resume() noexcept {
    if (timeout_) {
      errno = ETIMEDOUT;
      return {-1, {}};
    }
    socklen_t arrlen = sizeof(addr_);
    auto peer = accept4(fd_, reinterpret_cast<struct sockaddr*>(&addr_),
                        &arrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
    return {peer, IpAddress(addr_)};
  }

  void SetTimeout() {
    timeout_ = true;
    ioContext_->RemoveWriteIoEvent(fd_);
  }

  bool GetTimeout() const { return timeout_; }

 private:
  Base::IoContext* ioContext_;
  int fd_;
  struct sockaddr_in6 addr_;
  bool timeout_ = false;
};

class SendToAwaitable : public Base::AwaitableNonCopyable {
 public:
  SendToAwaitable(Base::IoContext* ioContext, int fd, const void* buf,
                  size_t len, const IpAddress& addr, int flags)
      : ioContext_(ioContext),
        fd_(fd),
        buf_(buf),
        len_(len),
        dest_(addr),
        addrlen_(dest_.IsIpv4() ? sizeof(struct sockaddr_in)
                                : sizeof(struct sockaddr_in6)),
        flags_(flags) {}

  bool await_ready() noexcept {
    retValue_ = sendto(fd_, buf_, len_, flags_, dest_.GetSockaddr(), addrlen_);
    if (retValue_ >= 0 || errno != EAGAIN) ready_ = true;
    return ready_;
  }

  void await_suspend(std::coroutine_handle<> handle) noexcept {
    assert(!ready_);
    ioContext_->AddWriteIoEvent(fd_, handle);
  }

  ssize_t await_resume() noexcept {
    if (timeout_) {
      errno = ETIMEDOUT;
      return -1;
    }
    if (ready_) return retValue_;
    return sendto(fd_, buf_, len_, flags_, dest_.GetSockaddr(), addrlen_);
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
  size_t len_;
  IpAddress dest_;
  socklen_t addrlen_;
  int flags_;
  bool ready_ = false;
  bool timeout_ = false;
  ssize_t retValue_;
};

class RecvFromAwaitable : public Base::AwaitableNonCopyable {
 public:
  RecvFromAwaitable(Base::IoContext* ioContext, int fd, void* buf, size_t len,
                    IpAddress& addr, int flags)
      : ioContext_(ioContext),
        fd_(fd),
        buf_(buf),
        len_(len),
        source_(&addr),
        addrlen_(sizeof(struct sockaddr_in6)),
        flags_(flags) {}

  bool await_ready() noexcept { return false; }

  void await_suspend(std::coroutine_handle<> handle) noexcept {
    ioContext_->AddReadIoEvent(fd_, handle);
  }

  ssize_t await_resume() noexcept {
    if (timeout_) {
      errno = ETIMEDOUT;
      return -1;
    }
    return recvfrom(fd_, buf_, len_, flags_, source_->GetSockaddr(), &addrlen_);
  }

  void SetTimeout() {
    timeout_ = true;
    ioContext_->RemoveWriteIoEvent(fd_);
  }

  bool GetTimeout() const { return timeout_; }

 private:
  Base::IoContext* ioContext_;
  int fd_;
  void* buf_;
  size_t len_;
  IpAddress* source_;
  socklen_t addrlen_;
  int flags_;
  bool timeout_ = false;
};

class ConnectAwaitable : public Base::AwaitableNonCopyable {
 public:
  ConnectAwaitable(Base::IoContext* ioContext, int fd, const IpAddress& ip,
                   std::atomic<bool>* connected, IpAddress* localAddress,
                   IpAddress* remoteAddress)
      : ioContext_(ioContext),
        fd_(fd),
        ipAddress_(ip),
        connected_(connected),
        localAddress_(localAddress),
        remoteAddress_(remoteAddress) {}

  bool await_ready() noexcept {
    retValue_ = connect(fd_, ipAddress_.GetSockaddr(),
                        ipAddress_.IsIpv4() ? sizeof(struct sockaddr_in)
                                            : sizeof(struct sockaddr_in6));
    if (retValue_ != -1 || errno != EINPROGRESS) notInprogress_ = true;
    return notInprogress_;
  }

  void await_suspend(std::coroutine_handle<> handle) noexcept {
    ioContext_->AddWriteIoEvent(fd_, handle);
  }

  int await_resume() noexcept {
    if (timeout_) {
      errno = ETIMEDOUT;
      return -1;
    }
    if (!notInprogress_) {
      socklen_t len = sizeof(int);
      if (getsockopt(fd_, SOL_SOCKET, SO_ERROR, &retValue_, &len) == -1)
        return -1;
    }
    if (retValue_ == 0) {
      assert(connected_);
      assert(localAddress_);
      assert(remoteAddress_);
      *connected_ = true;
      socklen_t len = sizeof(struct sockaddr_in6);
      getsockname(fd_, localAddress_->GetSockaddr(), &len);
      *remoteAddress_ = ipAddress_;
    }
    errno = retValue_;
    return retValue_;
  }

  void SetTimeout() {
    timeout_ = true;
    ioContext_->RemoveWriteIoEvent(fd_);
  }
  bool GetTimeout() const { return timeout_; }

 private:
  Base::IoContext* ioContext_;
  int fd_;
  IpAddress ipAddress_;
  std::atomic<bool>* connected_;
  IpAddress* localAddress_;
  IpAddress* remoteAddress_;
  int retValue_ = 0;
  bool notInprogress_ = false;
  bool timeout_ = false;
};

template <typename AWAITABLE, typename PERIOD, typename REP>
class IoTimeoutAwaitable : public Base::AwaitableNonCopyable {
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
