#ifndef COLD_NET_IOAWAITABLE
#define COLD_NET_IOAWAITABLE

#include <cerrno>
#include <chrono>

#include "cold/coro/IoService.h"
#include "cold/coro/Task.h"
#include "cold/net/IpAddress.h"
#include "cold/time/Timer.h"

namespace Cold::Net {

class IoAwaitableBase {
  template <typename AWAITABLE, typename REP, typename PERIOD>
  friend class IoTimeoutAwaitable;

 public:
  enum IoType { kREAD, kWRITE };
  IoAwaitableBase(Base::IoService* service, int fd, IoType type)
      : service_(service), fd_(fd), type_(type) {}

  virtual ~IoAwaitableBase() = default;

  void await_suspend(std::coroutine_handle<> handle) noexcept {
    ListenIo(handle);
  }

 protected:
  void ListenIo(const std::coroutine_handle<>& handle) {
    if (type_ == kREAD) {
      service_->ListenReadEvent(fd_, handle);
    } else {
      service_->ListenWriteEvent(fd_, handle);
    }
  }

  void StopListeningIo() {
    if (type_ == kREAD) {
      service_->StopListeningReadEvent(fd_);
    } else {
      service_->StopListeningWriteEvent(fd_);
    }
  }

  bool GetTimeout() const { return timeout_; }

  Base::IoService* service_;
  int fd_;

 private:
  void SetTimeout() {
    timeout_ = true;
    StopListeningIo();
  }

  IoType type_;
  bool timeout_ = false;
};

class ReadAwaitable : public IoAwaitableBase {
 public:
  ReadAwaitable(Base::IoService* service, int fd, void* buf, size_t count,
                std::atomic<bool>& connected)
      : IoAwaitableBase(service, fd, IoAwaitableBase::kREAD),
        buf_(buf),
        count_(count),
        connected_(connected) {}
  ~ReadAwaitable() override = default;

  bool await_ready() noexcept { return !connected_; }

  ssize_t await_resume() noexcept {
    if (!connected_ || GetTimeout()) {
      errno = GetTimeout() ? ETIMEDOUT : ENOTCONN;
      return -1;
    }
    auto n = read(fd_, buf_, count_);
    return n;
  }

 private:
  void* buf_;
  size_t count_;
  const std::atomic<bool>& connected_;
};

class WriteAwaitable : public IoAwaitableBase {
 public:
  WriteAwaitable(Base::IoService* service, int fd, const void* buf,
                 size_t count, std::atomic<bool>& connected)
      : IoAwaitableBase(service, fd, IoAwaitableBase::kWRITE),
        buf_(buf),
        count_(count),
        connected_(connected) {}

  ~WriteAwaitable() override = default;

  bool await_ready() noexcept {
    if (!connected_) return true;
    retValue_ = write(fd_, buf_, count_);
    if (retValue_ >= 0 || errno != EAGAIN) ready_ = true;
    return ready_;
  }

  ssize_t await_resume() noexcept {
    if (!connected_ || GetTimeout()) {
      errno = GetTimeout() ? ETIMEDOUT : ENOTCONN;
      return -1;
    }
    if (ready_) return retValue_;
    return write(fd_, buf_, count_);
  }

 private:
  const void* buf_;
  size_t count_;
  const std::atomic<bool>& connected_;
  bool ready_ = false;
  ssize_t retValue_ = -1;
};

class AcceptAwaitable : public IoAwaitableBase {
 public:
  AcceptAwaitable(Base::IoService* service, int fd)
      : IoAwaitableBase(service, fd, IoAwaitableBase::kREAD) {}

  ~AcceptAwaitable() override = default;

  bool await_ready() noexcept { return false; }

  std::pair<int, IpAddress> await_resume() noexcept {
    if (GetTimeout()) {
      errno = ETIMEDOUT;
      return {-1, IpAddress{}};
    }
    socklen_t arrlen = sizeof(addr_);
    auto peer = accept4(fd_, reinterpret_cast<struct sockaddr*>(&addr_),
                        &arrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
    return {peer, IpAddress(addr_)};
  }

 private:
  struct sockaddr_in6 addr_;
};

class SendToAwaitable : public IoAwaitableBase {
 public:
  SendToAwaitable(Base::IoService* service, int fd, const void* buf, size_t len,
                  const IpAddress& addr, int flags)
      : IoAwaitableBase(service, fd, IoAwaitableBase::kWRITE),
        buf_(buf),
        len_(len),
        dest_(addr),
        addrlen_(dest_.IsIpv4() ? sizeof(struct sockaddr_in)
                                : sizeof(struct sockaddr_in6)),
        flags_(flags) {}
  ~SendToAwaitable() override = default;

  bool await_ready() noexcept {
    retValue_ = sendto(fd_, buf_, len_, flags_, dest_.GetSockaddr(), addrlen_);
    if (retValue_ >= 0 || errno != EAGAIN) ready_ = true;
    return ready_;
  }

  ssize_t await_resume() noexcept {
    if (GetTimeout()) {
      errno = ETIMEDOUT;
      return -1;
    }
    if (ready_) return retValue_;
    return sendto(fd_, buf_, len_, flags_, dest_.GetSockaddr(), addrlen_);
  }

 private:
  const void* buf_;
  size_t len_;
  IpAddress dest_;
  socklen_t addrlen_;
  int flags_;
  bool ready_ = false;
  ssize_t retValue_;
};

class RecvFromAwaitable : public IoAwaitableBase {
 public:
  RecvFromAwaitable(Base::IoService* service, int fd, void* buf, size_t len,
                    IpAddress& addr, int flags)
      : IoAwaitableBase(service, fd, IoAwaitableBase::kREAD),
        buf_(buf),
        len_(len),
        source_(&addr),
        addrlen_(sizeof(struct sockaddr_in6)),
        flags_(flags) {}
  ~RecvFromAwaitable() override = default;

  bool await_ready() noexcept { return false; }

  ssize_t await_resume() noexcept {
    if (GetTimeout()) {
      errno = ETIMEDOUT;
      return -1;
    }
    return recvfrom(fd_, buf_, len_, flags_, source_->GetSockaddr(), &addrlen_);
  }

 private:
  void* buf_;
  size_t len_;
  IpAddress* source_;
  socklen_t addrlen_;
  int flags_;
};

class ConnectAwaitable : public IoAwaitableBase {
 public:
  ConnectAwaitable(Base::IoService* service, int fd, const IpAddress& ip,
                   std::atomic<bool>* connected, IpAddress* localAddress,
                   IpAddress* remoteAddress)
      : IoAwaitableBase(service, fd, IoAwaitableBase::kWRITE),
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

  int await_resume() noexcept {
    if (GetTimeout()) {
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

 private:
  IpAddress ipAddress_;
  std::atomic<bool>* connected_;
  IpAddress* localAddress_;
  IpAddress* remoteAddress_;
  int retValue_ = 0;
  bool notInprogress_ = false;
};

template <typename AWAITABLE, typename REP, typename PERIOD>
class IoTimeoutAwaitable {
 public:
  using Duration = std::chrono::duration<REP, PERIOD>;
  using RetType =
      std::invoke_result_t<decltype(&AWAITABLE::await_resume), AWAITABLE>;

  IoTimeoutAwaitable(Base::IoService* service, AWAITABLE&& awaitable,
                     Duration timeoutTime)
      : service_(service),
        awaitable_(std::move(awaitable)),
        timeoutTime_(timeoutTime),
        timer_(*service) {}
  ~IoTimeoutAwaitable() = default;

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
    service_->CoSpawn(std::move(task));
  }

  RetType await_resume() noexcept {
    if (!awaitable_.GetTimeout()) timer_.Cancel();
    return retValue_;
  }

 private:
  Base::IoService* service_;
  AWAITABLE awaitable_;
  Duration timeoutTime_;
  Base::Timer timer_;
  RetType retValue_;
};

}  // namespace Cold::Net

#endif /* COLD_NET_IOAWAITABLE */
