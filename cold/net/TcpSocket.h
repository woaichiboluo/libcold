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
  explicit TcpSocket(IoEvent* event) : BasicSocket(event, true) {
    connected_ = true;
  }

  ~TcpSocket() override = default;

  TcpSocket(TcpSocket&&) = default;
  TcpSocket& operator=(TcpSocket&&) = default;

  Task<bool> Connect(const IpAddress& address) {
    assert(IsValid());
    if (co_await BasicSocket::Connect(address) == 0) {
      event_->EnableReadingET();
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
    assert(IsValid() && IsConnected());
    if (sslEnabled_) {
      co_return co_await detail::ReadAwaitable(event_, CanReading(), buffer,
                                               size);
    } else {
      co_return co_await detail::ReadAwaitable(event_, CanReading(), buffer,
                                               size);
    }
  }

  [[nodiscard]] Task<ssize_t> Write(const void* buffer, size_t size) {
    assert(IsValid() && IsConnected());
    if (sslEnabled_) {
      co_return co_await detail::WriteAwaitable(event_, CanWriting(), buffer,
                                                size);
    } else {
      co_return co_await detail::WriteAwaitable(event_, CanWriting(), buffer,
                                                size);
    }
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
