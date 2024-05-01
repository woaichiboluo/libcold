#ifndef COLD_NET_TCPSOCKET
#define COLD_NET_TCPSOCKET

#include <atomic>

#include "cold/coro/IoService.h"
#include "cold/log/Logger.h"
#include "cold/net/BasicSocket.h"

namespace Cold::Net {

class TcpSocket : public Net::BasicSocket {
 public:
  TcpSocket() = default;

  TcpSocket(Base::IoService& service, bool ipv6 = false)
      : Net::BasicSocket(
            service, socket(ipv6 ? AF_INET6 : AF_INET,
                            SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0)) {
    if (fd_ < 0) {
      Base::ERROR("create TcpSocket error. errno: {} Reason: {}", errno,
                  Base::ThisThread::ErrorMsg());
    }
  }

  TcpSocket(Base::IoService& service, const IpAddress& local,
            const IpAddress& remote, int fd)
      : BasicSocket(service, local, remote, fd) {
    if (fd >= 0) connected_ = true;
  }

  TcpSocket(TcpSocket&& other) = default;
  TcpSocket& operator=(TcpSocket&& other) = default;

  ~TcpSocket() override = default;

  void Close() override {
    BasicSocket::Close();
    connected_ = false;
  }
};

}  // namespace Cold::Net

#endif /* COLD_NET_TCPSOCKET */
