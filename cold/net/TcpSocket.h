#ifndef COLD_NET_TCPSOCKET
#define COLD_NET_TCPSOCKET

#include <atomic>

#include "cold/coro/IoService.h"
#include "cold/log/Logger.h"
#include "cold/net/BasicSocket.h"
#include "cold/util/Config.h"

namespace Cold::Net {

class TcpSocket : public Net::BasicSocket {
 public:
  TcpSocket() = default;

  TcpSocket(Base::IoService& service, bool enableSSL = false, bool ipv6 = false)
      : Net::BasicSocket(service,
                         socket(ipv6 ? AF_INET6 : AF_INET,
                                SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0),
                         enableSSL) {
    if (fd_ < 0) {
      Base::ERROR("create TcpSocket error. errno: {} Reason: {}", errno,
                  Base::ThisThread::ErrorMsg());
    }
  }

  TcpSocket(Base::IoService& service, const IpAddress& local,
            const IpAddress& remote, int fd, SSL* ssl = nullptr)
      : BasicSocket(service, local, remote, fd, ssl) {
    if (fd >= 0) connected_ = true;
  }

  TcpSocket(TcpSocket&& other) = default;
  TcpSocket& operator=(TcpSocket&& other) = default;

  ~TcpSocket() override = default;

  void Close() override {
    BasicSocket::Close();
    connected_ = false;
  }

  // for debug
  SSL* GetSSL() const { return ssl_; }

  [[nodiscard]] Base::Task<ssize_t> ReadN(char* buf, size_t n) {
    size_t byteAlreadyRead = 0;
    while (byteAlreadyRead < n) {
      auto ret = co_await Read(buf + byteAlreadyRead, n - byteAlreadyRead);
      if (ret <= 0) co_return ret;
      byteAlreadyRead += static_cast<size_t>(ret);
    }
    co_return static_cast<ssize_t>(n);
  }

  [[nodiscard]] Base::Task<ssize_t> WriteN(const char* buf, size_t n) {
    ioService_->CoSpawn(
        [](TcpSocket* socket) -> Base::Task<> { co_return; }(this));
    size_t byteAlreadyWrite = 0;
    while (byteAlreadyWrite < n) {
      auto ret = co_await Write(buf + byteAlreadyWrite, n - byteAlreadyWrite);
      if (ret < 0) co_return ret;
      byteAlreadyWrite += static_cast<size_t>(ret);
    }
    co_return static_cast<ssize_t>(n);
  }

  template <typename REP, typename PERIOD>
  [[nodiscard]] Base::Task<ssize_t> WriteNWithTimeout(
      const char* buf, size_t n, std::chrono::duration<REP, PERIOD> duration) {
    ioService_->CoSpawn(
        [](TcpSocket* socket) -> Base::Task<> { co_return; }(this));
    size_t byteAlreadyWrite = 0;
    while (byteAlreadyWrite < n) {
      auto ret = co_await WriteWithTimeout(buf + byteAlreadyWrite,
                                           n - byteAlreadyWrite, duration);
      if (ret < 0) co_return ret;
      byteAlreadyWrite += static_cast<size_t>(ret);
    }
    co_return static_cast<ssize_t>(n);
  }

#ifdef COLD_NET_ENABLE_SSL
  [[nodiscard]] Base::Task<bool> DoHandshake() {
    assert(ssl_);
    SSL_set_connect_state(ssl_);
    while (true) {
      auto ret = co_await Net::IoTimeoutAwaitable(
          ioService_, Net::HandshakeAwaitable(ioService_, fd_, ssl_),
          std::chrono::seconds(
              Base::Config::GetGloablDefaultConfig().GetOrDefault<int>(
                  "/ssl/client-handshake-timeout", 5)));
      if (ret == SSL_ERROR_NONE) break;
      if (ret != SSL_ERROR_WANT_READ && ret != SSL_ERROR_WANT_WRITE) {
        co_return false;
      }
    }
    co_return true;
  }
#endif
};

}  // namespace Cold::Net

#endif /* COLD_NET_TCPSOCKET */
