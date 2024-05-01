#ifndef COLD_NET_IOAWAITABLE
#define COLD_NET_IOAWAITABLE

#include <coroutine>

#include "cold/coro/IoService.h"
#include "cold/coro/IoWatcher.h"
#include "cold/net/IpAddress.h"

namespace Cold::Net {

class IoAwaitableBase {
 public:
  enum IoType { READ, WRITE };
  IoAwaitableBase(Base::IoService* service, int fd, IoType type)
      : service_(service), fd_(fd), type_(type) {}

  virtual ~IoAwaitableBase() = default;

  void await_suspend(std::coroutine_handle<> handle) noexcept {
    ListenIo(handle);
  }

 protected:
  void ListenIo(const std::coroutine_handle<>& handle) {
    if (type_ == READ) {
      service_->GetIoWatcher()->ListenReadEvent(fd_, handle);
    } else {
      service_->GetIoWatcher()->ListenWriteEvent(fd_, handle);
    }
  }

  void StopListeningIo() {
    if (type_ == READ) {
      service_->GetIoWatcher()->StopListeningReadEvent(fd_);
    } else {
      service_->GetIoWatcher()->StopListeningWriteEvent(fd_);
    }
  }

  Base::IoService* service_;
  int fd_;

 private:
  IoType type_;
};

class ReadAwaitable : public IoAwaitableBase {
 public:
  ReadAwaitable(Base::IoService* service, int fd, void* buf, size_t count,
                std::atomic<bool>& connected)
      : IoAwaitableBase(service, fd, IoAwaitableBase::READ),
        buf_(buf),
        count_(count),
        connected_(connected) {}
  ~ReadAwaitable() override = default;

  bool await_ready() noexcept { return !connected_; }

  ssize_t await_resume() noexcept {
    if (!connected_) {
      errno = ENOTCONN;
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
      : IoAwaitableBase(service, fd, IoAwaitableBase::WRITE),
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
    if (!connected_) {
      errno = ENOTCONN;
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
      : IoAwaitableBase(service, fd, IoAwaitableBase::READ) {}

  ~AcceptAwaitable() override = default;

  bool await_ready() noexcept { return false; }

  std::pair<int, IpAddress> await_resume() noexcept {
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
      : IoAwaitableBase(service, fd, IoAwaitableBase::WRITE),
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
      : IoAwaitableBase(service, fd, IoAwaitableBase::READ),
        buf_(buf),
        len_(len),
        source_(&addr),
        addrlen_(sizeof(struct sockaddr_in6)),
        flags_(flags) {}
  ~RecvFromAwaitable() override = default;

  bool await_ready() noexcept { return false; }

  ssize_t await_resume() noexcept {
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
      : IoAwaitableBase(service, fd, IoAwaitableBase::WRITE),
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

}  // namespace Cold::Net

#endif /* COLD_NET_IOAWAITABLE */
