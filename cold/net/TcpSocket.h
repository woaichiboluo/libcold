#ifndef COLD_NET_TCPSOCKET
#define COLD_NET_TCPSOCKET

#include <netinet/tcp.h>

#include "BasicSocket.h"

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

  ~TcpSocket() override = default;

  TcpSocket(TcpSocket&&) = default;
  TcpSocket& operator=(TcpSocket&&) = default;

  Task<bool> Connect(const IpAddress& address) {
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
      co_return true;
    }
    co_return false;
  }

  bool TcpNoDelay(bool enable) {
    int flag = enable ? 1 : 0;
    return setsockopt(event_->GetFd(), IPPROTO_TCP, TCP_NODELAY, &flag,
                      sizeof(flag)) == 0;
  }

  [[nodiscard]] Task<ssize_t> Read(void* buffer, size_t size) {
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

  void EnableSSL() { sslEnabled_ = true; }

 protected:
  bool sslEnabled_ = false;
};

}  // namespace Cold

#endif /* COLD_NET_TCPSOCKET */
