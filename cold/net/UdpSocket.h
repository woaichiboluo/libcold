#ifndef COLD_NET_UDPSOCKET
#define COLD_NET_UDPSOCKET

#include <cassert>

#include "cold/log/Logger.h"
#include "cold/net/BasicSocket.h"
#include "cold/net/IoAwaitable.h"

namespace Cold::Net {

class UdpSocket : public BasicSocket {
 public:
  UdpSocket() = default;

  UdpSocket(Base::IoService& service, bool ipv6 = false)
      : Net::BasicSocket(service,
                         socket(ipv6 ? AF_INET6 : AF_INET,
                                SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0),
                         false) {
    if (fd_ < 0) {
      Base::ERROR("create TcpSocket error. errno: {}, reason: {}", errno,
                  Base::ThisThread::ErrorMsg());
    }

    ioService_->ListenReadEvent(fd_, std::noop_coroutine());
  }

  UdpSocket(UdpSocket&&) = default;
  UdpSocket& operator=(UdpSocket&&) = default;
  ~UdpSocket() override = default;

  auto SendTo(const void* buf, size_t len, const IpAddress& dest,
              int flags = 0) {
    assert(!connected_);
    return SendToAwaitable(ioService_, fd_, buf, len, dest, flags);
  }

  auto RecvFrom(void* buf, size_t len, IpAddress& source, int flags = 0) {
    assert(!connected_);
    return RecvFromAwaitable(ioService_, fd_, buf, len, source, flags);
  }

  template <typename REP, typename PERIOD>
  auto SendToWithTimeout(const void* buf, size_t len, const IpAddress& dest,
                         std::chrono::duration<REP, PERIOD> duration,
                         int flags = 0) {
    return IoTimeoutAwaitable(ioService_, SendTo(buf, len, dest, flags),
                              duration);
  }

  template <typename REP, typename PERIOD>
  auto RecvFromWithTimeout(void* buf, size_t len, IpAddress& source,
                           std::chrono::duration<REP, PERIOD> duration,
                           int flags = 0) {
    return IoTimeoutAwaitable(ioService_, RecvFrom(buf, len, source, flags),
                              duration);
  }
};

}  // namespace Cold::Net

#endif /* COLD_NET_UDPSOCKET */
