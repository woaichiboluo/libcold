#ifndef COLD_NET_UDPSOCKET
#define COLD_NET_UDPSOCKET

#include "cold/log/Logger.h"
#include "cold/net/BasicSocket.h"

namespace Cold::Net {

class UdpSocket : public BasicSocket {
 public:
  UdpSocket() = default;

  UdpSocket(Base::IoService& service, bool ipv6 = false)
      : Net::BasicSocket(service,
                         socket(ipv6 ? AF_INET6 : AF_INET,
                                SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0)) {
    if (fd_ < 0) {
      Base::ERROR("create TcpSocket error. errno: {}, reason: {}", errno,
                  Base::ThisThread::ErrorMsg());
    }
  }

  UdpSocket(UdpSocket&&) = default;
  UdpSocket& operator=(UdpSocket&&) = default;
  ~UdpSocket() override = default;

  auto SendTo(const void* buf, size_t len, const IpAddress& dest,
              int flags = 0) {
    return SendToAwaitable(ioService_, fd_, buf, len, dest, flags);
  }

  auto RecvFrom(void* buf, size_t len, IpAddress& source, int flags = 0) {
    assert(!connected_);
    return RecvFromAwaitable(ioService_, fd_, buf, len, source, flags);
  }
};

}  // namespace Cold::Net

#endif /* COLD_NET_UDPSOCKET */
