#ifndef COLD_NET_IOAWAITABLE
#define COLD_NET_IOAWAITABLE

#include <cerrno>

#include "cold/coro/Io.h"
#include "cold/net/IpAddress.h"
#include "sys/sendfile.h"

#ifdef COLD_NET_ENABLE_SSL
#include <openssl/ssl.h>
#else
class SSL;
#endif

namespace Cold::Net {

using Base::IoAwaitableBase;
using Base::IoTimeoutAwaitable;

#ifdef COLD_NET_ENABLE_SSL
class SSLReadAwaitable : public IoAwaitableBase {
 public:
  SSLReadAwaitable(Base::IoService* service, SSL* ssl, void* buf, size_t count,
                   const std::atomic<bool>& connected)
      : IoAwaitableBase(service, SSL_get_fd(ssl), IoAwaitableBase::kREAD),
        ssl_(ssl),
        buf_(buf),
        count_(count),
        connected_(connected) {}

  ~SSLReadAwaitable() override = default;

  bool await_ready() noexcept {
    retValue_ = SSL_read(ssl_, buf_, static_cast<int>(count_));
    if (retValue_ >= 0) {
      ready_ = true;
    } else {
      int e = SSL_get_error(ssl_, static_cast<int>(retValue_));
      if (e != SSL_ERROR_WANT_READ) ready_ = true;
    }
    return ready_;
  }

  ssize_t await_resume() noexcept {
    if (!connected_ || GetTimeout()) {
      errno = GetTimeout() ? ETIMEDOUT : ENOTCONN;
      return -1;
    }
    if (ready_) return retValue_;
    return SSL_read(ssl_, buf_, static_cast<int>(count_));
  }

 private:
  SSL* ssl_;
  void* buf_;
  size_t count_;
  bool ready_ = false;
  ssize_t retValue_ = 0;
  const std::atomic<bool>& connected_;
};
#endif

class ReadAwaitable : public IoAwaitableBase {
 public:
  ReadAwaitable(Base::IoService* service, int fd, void* buf, size_t count,
                std::atomic<bool>& connected, SSL* ssl)
      : IoAwaitableBase(service, fd, IoAwaitableBase::kREAD),
        buf_(buf),
        count_(count),
        connected_(connected),
        ssl_(ssl),
        states_(std::make_shared<std::pair<ssize_t, bool>>(0, false)) {
    (void)ssl_;
  }
  ~ReadAwaitable() override = default;

  bool await_ready() noexcept {
    if (!connected_) return true;
    if (ssl_) return false;
    states_->first = read(fd_, buf_, count_);
    if (states_->first >= 0 || errno != EAGAIN) ready_ = true;
    return ready_;
  }

  void await_suspend(std::coroutine_handle<> handle) noexcept {
    if (!ssl_) {
      IoAwaitableBase::await_suspend(handle);
    } else {
#ifdef COLD_NET_ENABLE_SSL
      service_->CoSpawn(
          [](std::coroutine_handle<> h, Base::IoService* service, SSL* ssl,
             void* buf, size_t count, const std::atomic<bool>& connected,
             std::shared_ptr<std::pair<ssize_t, bool>> states) -> Base::Task<> {
            while (!states->second) {
              states->first = co_await SSLReadAwaitable(service, ssl, buf,
                                                        count, connected);
              if (states->first == -1 &&
                  SSL_get_error(ssl, static_cast<int>(states->first)) ==
                      SSL_ERROR_WANT_READ) {
                states->first = 0;
                continue;
              }
              break;
            }
            if (!states->second) h.resume();
          }(handle, service_, ssl_, buf_, count_, connected_, states_));
#endif
    }
  }

  ssize_t await_resume() noexcept {
    states_->second = true;
    if (!connected_ || GetTimeout()) {
      errno = GetTimeout() ? ETIMEDOUT : ENOTCONN;
      return -1;
    }
    if (!ssl_ && !ready_) states_->first = read(fd_, buf_, count_);
    return states_->first;
  }

 private:
  void* buf_;
  size_t count_;
  bool ready_ = false;
  const std::atomic<bool>& connected_;
  SSL* ssl_;
  // first retValue second alreadyResume
  std::shared_ptr<std::pair<ssize_t, bool>> states_;
};

class WriteAwaitable : public IoAwaitableBase {
 public:
  WriteAwaitable(Base::IoService* service, int fd, const void* buf,
                 size_t count, std::atomic<bool>& connected, SSL* ssl)
      : IoAwaitableBase(service, fd, IoAwaitableBase::kWRITE),
        buf_(buf),
        count_(count),
        connected_(connected),
        ssl_(ssl) {
    (void)ssl_;
  }

  ~WriteAwaitable() override = default;

  bool await_ready() noexcept {
    if (!connected_) return true;
#ifdef COLD_NET_ENABLE_SSL
    if (ssl_) {
      retValue_ = SSL_write(ssl_, buf_, static_cast<int>(count_));
      if (retValue_ > 0) {
        ready_ = true;
      } else {
        int e = SSL_get_error(ssl_, static_cast<int>(retValue_));
        if (e != SSL_ERROR_WANT_WRITE) ready_ = true;
      }
      return ready_;
    }
#endif
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
#ifdef COLD_NET_ENABLE_SSL
    if (ssl_) {
      return SSL_write(ssl_, buf_, static_cast<int>(count_));
    }
#endif
    return write(fd_, buf_, count_);
  }

 private:
  const void* buf_;
  size_t count_;
  const std::atomic<bool>& connected_;
  bool ready_ = false;
  ssize_t retValue_ = -1;
  SSL* ssl_;
};

class AcceptAwaitable : public IoAwaitableBase {
 public:
  AcceptAwaitable(Base::IoService* service, int fd)
      : IoAwaitableBase(service, fd, IoAwaitableBase::kREAD) {}

  ~AcceptAwaitable() override = default;

  bool await_ready() noexcept {
    socklen_t arrlen = sizeof(addr_);
    peer_ = accept4(fd_, reinterpret_cast<struct sockaddr*>(&addr_), &arrlen,
                    SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (peer_ >= 0) {
      ready_ = true;
    }
    return ready_;
  }

  std::pair<int, IpAddress> await_resume() noexcept {
    if (GetTimeout()) {
      errno = ETIMEDOUT;
      return {-1, IpAddress{}};
    }
    if (!ready_) {
      socklen_t arrlen = sizeof(addr_);
      peer_ = accept4(fd_, reinterpret_cast<struct sockaddr*>(&addr_), &arrlen,
                      SOCK_NONBLOCK | SOCK_CLOEXEC);
    }
    return {peer_, IpAddress(addr_)};
  }

 private:
  struct sockaddr_in6 addr_;
  bool ready_ = false;
  int peer_ = -1;
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

  bool await_ready() noexcept {
    retValue_ =
        recvfrom(fd_, buf_, len_, flags_, source_->GetSockaddr(), &addrlen_);
    if (retValue_ >= 0 || errno != EAGAIN) ready_ = true;
    return false;
  }

  ssize_t await_resume() noexcept {
    if (GetTimeout()) {
      errno = ETIMEDOUT;
      return -1;
    }
    if (ready_) return retValue_;
    return recvfrom(fd_, buf_, len_, flags_, source_->GetSockaddr(), &addrlen_);
  }

 private:
  void* buf_;
  size_t len_;
  bool ready_ = false;
  ssize_t retValue_;
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
    } else {
      errno = retValue_;
      retValue_ = -1;
    }
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

class SendFileAwaitable : public IoAwaitableBase {
 public:
  SendFileAwaitable(Base::IoService* service, int outFd, int inFd,
                    off_t* offset, size_t count)
      : IoAwaitableBase(service, outFd, IoAwaitableBase::kWRITE),
        inFd_(inFd),
        offset_(offset),
        count_(count) {}

  bool await_ready() noexcept {
    retValue_ = sendfile(fd_, inFd_, offset_, count_);
    if (retValue_ >= 0 || errno != EAGAIN) ready_ = true;
    return ready_;
  }

  ssize_t await_resume() noexcept {
    if (GetTimeout()) {
      errno = ETIMEDOUT;
      return -1;
    }
    if (ready_) return retValue_;
    return sendfile(fd_, inFd_, offset_, count_);
  }

 private:
  int inFd_;
  off_t* offset_;
  size_t count_;
  bool ready_ = false;
  ssize_t retValue_ = 0;
};

#ifdef COLD_NET_ENABLE_SSL

class HandshakeAwaitable : public IoAwaitableBase {
 public:
  HandshakeAwaitable(Base::IoService* service, int fd, SSL* ssl)
      : IoAwaitableBase(service, fd, IoType::kREAD), ssl_(ssl) {}
  ~HandshakeAwaitable() override = default;

  bool GetTimeout() const { return timeout_; }

  void SetTimeout() {
    timeout_ = true;
    StopListeningIo();
  }

  bool await_ready() noexcept {
    retValue_ = SSL_do_handshake(ssl_);
    if (retValue_ == 1) return true;
    error_ = SSL_get_error(ssl_, retValue_);
    if (error_ == SSL_ERROR_WANT_READ) {
      SetIoType(IoType::kREAD);
    } else if (error_ == SSL_ERROR_WANT_WRITE) {
      SetIoType(IoType::kWRITE);
    } else {
      return true;
    }
    return false;
  }

  int await_resume() noexcept {
    if (GetTimeout()) {
      errno = ETIMEDOUT;
      return -1;
    }
    return retValue_ == 1 ? SSL_ERROR_NONE : error_;
  }

 private:
  SSL* ssl_;
  int retValue_ = 0;
  int error_ = 0;
  bool timeout_ = false;
};

#endif

}  // namespace Cold::Net

#endif /* COLD_NET_IOAWAITABLE */
