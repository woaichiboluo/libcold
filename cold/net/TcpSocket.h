#ifndef COLD_NET_TCPSOCKET
#define COLD_NET_TCPSOCKET

#include <atomic>

#include "cold/log/Logger.h"
#include "cold/net/BasicSocket.h"

namespace Cold::Net {

class TcpSocket : public Net::BasicSocket {
 public:
  TcpSocket(Base::IoContext& ioContext, bool ipv6 = false)
      : Net::BasicSocket(
            ioContext, socket(ipv6 ? AF_INET6 : AF_INET,
                              SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0)) {
    if (fd_ < 0) {
      LOG_ERROR(Base::GetMainLogger(),
                "create TcpSocket error. errno:{} Reason:{}", errno,
                Base::ThisThread::ErrorMsg());
    }
  }

  TcpSocket(Base::IoContext& ioContext, const IpAddress& local,
            const IpAddress& remote, int fd)
      : BasicSocket(ioContext, local, remote, fd) {
    // FIXME check fd nonblocking
    connected_ = true;
  }

  TcpSocket(TcpSocket&& other) = default;
  TcpSocket& operator=(TcpSocket&& other) = default;

  ~TcpSocket() override = default;

  void Close() override {
    connected_ = false;
    BasicSocket::Close();
  }
};

}  // namespace Cold::Net

#endif /* COLD_NET_TCPSOCKET */
