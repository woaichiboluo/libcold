#ifndef COLD_NET_UDPSOCKET
#define COLD_NET_UDPSOCKET

#include "cold/log/Logger.h"
#include "cold/net/BasicSocket.h"

namespace Cold::Net {

class UdpSocket : public BasicSocket {
 public:
  UdpSocket(Base::IoContext& ioContext, bool ipv6 = false)
      : Net::BasicSocket(ioContext,
                         socket(ipv6 ? AF_INET6 : AF_INET,
                                SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0)) {
    if (fd_ < 0) {
      LOG_ERROR(Base::GetMainLogger(),
                "create TcpSocket error. errno:{} Reason:{}", errno,
                Base::ThisThread::ErrorMsg());
    }
  }

  UdpSocket(UdpSocket&&) = default;
  UdpSocket& operator=(UdpSocket&&) = default;
  ~UdpSocket() override = default;

  auto SendTo(const void* buf, size_t len, const IpAddress& dest,
              int flags = 0) {
    return SendToAwaitable(ioContext_, fd_, buf, len, dest, flags);
  }

  template <typename PERIOD, typename REP>
  void SendToWithTimeout(const void* buf, size_t len, const IpAddress& dest,
                         std::chrono::duration<PERIOD, REP> duration,
                         int flags = 0) {
    assert(!connected_);
    return IoTimeoutAwaitable(ioContext_, SendTo(buf, len, dest), duration);
  }

  auto RecvFrom(void* buf, size_t len, IpAddress& source, int flags = 0) {
    assert(!connected_);
    return RecvFromAwaitable(ioContext_, fd_, buf, len, source, flags);
  }

  template <typename PERIOD, typename REP>
  auto RecvFromWithTimeout(void* buf, size_t len, IpAddress& source,
                           std::chrono::duration<PERIOD, REP> duration,
                           int flags = 0) {
    return IoTimeoutAwaitable(ioContext_, RecvFrom(buf, len, source, flags),
                              duration);
  }
};

}  // namespace Cold::Net

#endif /* COLD_NET_UDPSOCKET */
