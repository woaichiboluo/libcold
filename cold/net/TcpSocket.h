#ifndef COLD_NET_TCPSOCKET
#define COLD_NET_TCPSOCKET

#include <netinet/tcp.h>

#include "../detail/IoAwaitable.h"
#include "BasicSocket.h"

#ifdef COLD_ENABLE_SSL
#include "SSLContext.h"
#endif

namespace Cold {

class TcpSocket : public BasicSocket {
 public:
  // for unvalid socket
  TcpSocket() = default;

  // for client
  explicit TcpSocket(IoContext& ioContext, bool ipv6 = false)
      : BasicSocket(ioContext, ipv6, true) {}

  // for server
  explicit TcpSocket(std::shared_ptr<Detail::IoEvent> event)
      : BasicSocket(event, true) {
    connected_ = true;
  }

#ifdef COLD_ENABLE_SSL
  // for server
  TcpSocket(std::shared_ptr<Detail::IoEvent> event, SSL* ssl)
      : BasicSocket(event, true), sslEnabled_(true), sslHodler_(ssl) {
    connected_ = true;
  }
#endif

  ~TcpSocket() override = default;

  TcpSocket(TcpSocket&&) = default;
  TcpSocket& operator=(TcpSocket&&) = default;

#ifdef COLD_ENABLE_SSL
  Task<bool> Connect(IpAddress address) {
    if (sslEnabled_) {
      co_return co_await Connect(address,
                                 SSLContext::GetGlobalDefaultInstance());
    } else {
      auto ret = co_await ConnectInternal(address);
      if (ret) event_->EnableReading(true);
      co_return ret;
    }
  }

  Task<bool> Connect(IpAddress address, SSLContext& sslContext) {
    auto ret = co_await ConnectInternal(address);
    if (ret) {
      SSL* ssl = co_await Handshake(event_, sslContext, false);
      if (!ssl) {
        Close();
        co_return false;
      }
      sslHodler_ = SSLHolder(ssl);
      event_->EnableReading(true);
    }
    co_return ret;
  }

#else
  Task<bool> Connect(IpAddress address) {
    auto ret = co_await ConnectInternal(address);
    if (ret) event_->EnableReading(true);
    co_return ret;
  }
#endif

  bool TcpNoDelay(bool enable) {
    int flag = enable ? 1 : 0;
    return setsockopt(event_->GetFd(), IPPROTO_TCP, TCP_NODELAY, &flag,
                      sizeof(flag)) == 0;
  }

  [[nodiscard]] Task<ssize_t> Read(void* buffer, size_t size) {
#ifdef COLD_ENABLE_SSL
    if (sslEnabled_) {
      co_return co_await SSLRead(buffer, size);
    }
#endif
    while (CanReading()) {
      auto ret = read(event_->GetFd(), buffer, size);
      if (ret >= 0) co_return ret;
      if (errno == EAGAIN) {
        co_await Detail::ReadIoAwaitable(event_, false);
        continue;
      }
      co_return ret;
    }
    errno = ENOTCONN;
    co_return -1;
  }

  [[nodiscard]] Task<ssize_t> Write(const void* buffer, size_t size) {
#ifdef COLD_ENABLE_SSL
    if (sslEnabled_) {
      co_return co_await SSLWrite(buffer, size);
    }
#endif
    while (CanWriting()) {
      auto ret = write(event_->GetFd(), buffer, size);
      if (ret >= 0) co_return ret;
      if (errno == EAGAIN) {
        co_await Detail::WriteIoAwaitable(event_, true);
        continue;
      }
      co_return ret;
    }
    errno = ENOTCONN;
    co_return -1;
  }

  [[nodiscard]] Task<ssize_t> ReadN(void* buffer, size_t size) {
    size_t byteAlreadyRead = 0;
    while (byteAlreadyRead < size) {
      ssize_t ret = co_await Read(static_cast<char*>(buffer) + byteAlreadyRead,
                                  size - byteAlreadyRead);
      if (ret <= 0) co_return ret;
      byteAlreadyRead += static_cast<size_t>(ret);
    }
    co_return static_cast<ssize_t>(size);
  }

  [[nodiscard]] Task<ssize_t> WriteN(const void* buffer, size_t size) {
    size_t byteAlreadyWrite = 0;
    while (byteAlreadyWrite < size) {
      ssize_t ret =
          co_await Write(static_cast<const char*>(buffer) + byteAlreadyWrite,
                         size - byteAlreadyWrite);
      if (ret <= 0) co_return ret;
      byteAlreadyWrite += static_cast<size_t>(ret);
    }
    co_return static_cast<ssize_t>(size);
  }

#ifdef COLD_ENABLE_SSL
  void EnableSSL() { sslEnabled_ = true; }

  void Close() {
    if (sslHodler_.GetNativeHandle()) {
      SSL_shutdown(sslHodler_.GetNativeHandle());
    }
    BasicSocket::Close();
  }

  SSL* GetSSLNativeHandle() const { return sslHodler_.GetNativeHandle(); }

 protected:
  Task<ssize_t> SSLRead(void* buffer, size_t size) {
    while (CanReading()) {
      auto ret = SSL_read(sslHodler_.GetNativeHandle(), buffer,
                          static_cast<int>(size));
      int err = sslHodler_.GetSSLErrorCode(ret);
      if (err == SSL_ERROR_NONE) {
        co_return ret;
      } else {
        ERR_clear_error();
        if (err == SSL_ERROR_WANT_READ) {
          co_await Detail::ReadIoAwaitable(event_, false);
        } else if (err == SSL_ERROR_ZERO_RETURN) {
          co_return 0;
        } else {
          co_return -1;
        }
      }
    }
    errno = ENOTCONN;
    co_return -1;
  }

  Task<ssize_t> SSLWrite(const void* buffer, size_t size) {
    assert(size);
    while (CanWriting()) {
      ERR_clear_error();
      auto ret = SSL_write(sslHodler_.GetNativeHandle(), buffer,
                           static_cast<int>(size));
      int err = sslHodler_.GetSSLErrorCode(ret);
      if (err == SSL_ERROR_NONE) {
        co_return ret;
      } else {
        ERR_clear_error();
        if (err == SSL_ERROR_WANT_WRITE) {
          co_await Detail::WriteIoAwaitable(event_, true);
        } else {
          co_return -1;
        }
      }
    }
    errno = ENOTCONN;
    co_return -1;
  }

  static Task<SSL*> Handshake(std::shared_ptr<Detail::IoEvent> ev,
                              SSLContext& sslContext, bool accept) {
    bool error = false;
    auto ssl = SSL_new(sslContext.GetNativeHandle());
    ScopeGuard guard([&]() {
      if (!error || !ssl) return;
      ERR_clear_error();
      SSL_free(ssl);
    });
    if (!ssl) {
      error = true;
      DEBUG("SSL_new failed. {}", SSLError::GetLastErrorStr());
      co_return nullptr;
    }
    if (!SSL_set_fd(ssl, ev->GetFd())) {
      error = true;
      DEBUG("SSL_set_fd failed. {}", SSLError::GetLastErrorStr());
      co_return nullptr;
    }
    if (accept) {
      SSL_set_accept_state(ssl);
    } else {
      SSL_set_connect_state(ssl);
    }
    while (true) {
      auto ret = SSL_do_handshake(ssl);
      if (ret == 1) break;
      auto err = SSLError::GetSSLErrorCode(ssl, ret);
      if (err == SSL_ERROR_WANT_READ) {
        co_await Detail::ReadIoAwaitable(ev, true);
      } else if (err == SSL_ERROR_WANT_WRITE) {
        co_await Detail::WriteIoAwaitable(ev, true);
      } else {
        error = true;
        co_return nullptr;
      }
    }
    assert(!error && ssl);
    co_return ssl;
  }

  bool sslEnabled_ = false;
  SSLHolder sslHodler_;

#endif
 private:
  Task<bool> ConnectInternal(IpAddress address) {
    assert(IsValid() && !connected_);
    auto ret =
        connect(event_->GetFd(), address.GetSockaddr(), address.GetSocklen());
    if (ret == 0) co_return ret;
    if (ret < 0 && errno != EINPROGRESS) co_return false;
    assert(ret == -1 && errno == EINPROGRESS);
    co_await Detail::WriteIoAwaitable(event_, true);
    int opt = 0;
    socklen_t len = sizeof(opt);
    if (getsockopt(event_->GetFd(), SOL_SOCKET, SO_ERROR, &opt, &len) == 0) {
      errno = opt;
      if (opt != 0) {
        co_return false;
      }
      SetRemoteAddress(address);
      sockaddr_in6 local;
      len = sizeof(local);
      getsockname(event_->GetFd(), reinterpret_cast<sockaddr*>(&local), &len);
      SetLocalAddress(IpAddress(local));
      connected_ = true;
      co_return true;
    }
    co_return false;
  }
};

}  // namespace Cold

#endif /* COLD_NET_TCPSOCKET */
