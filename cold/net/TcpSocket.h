#ifndef COLD_NET_TCPSOCKET
#define COLD_NET_TCPSOCKET

#include <atomic>

#include "cold/log/Logger.h"
#include "cold/net/BasicSocket.h"
#include "cold/net/IoAwaitable.h"

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
    connected_ = true;
  }

  TcpSocket(TcpSocket&& other)
      : BasicSocket(std::move(other)), connected_(other.connected_.load()) {
    other.connected_ = false;
  }

  TcpSocket& operator=(TcpSocket&& other) {
    if (this == &other) return *this;
    BasicSocket::operator=(std::move(other));
    connected_ = other.connected_.load();
    other.connected_ = false;
    return *this;
  }

  ~TcpSocket() override = default;

  auto Read(void* buf, size_t count) {
    return ReadAwaitable(ioContext_, fd_, buf, count, connected_);
  }

  auto Write(const void* buf, size_t count) {
    return WriteAwaitable(ioContext_, fd_, buf, count, connected_);
  }

  template <typename PERIOD, typename REP>
  auto ReadWithTimeout(void* buf, size_t count,
                       std::chrono::duration<PERIOD, REP> duration) {
    return IoTimeoutAwaitable(ioContext_, Read(buf, count), duration);
  }

  template <typename PERIOD, typename REP>
  auto WriteWithTimeout(const void* buf, size_t count,
                        std::chrono::duration<PERIOD, REP> duration) {
    return IoTimeoutAwaitable(ioContext_, Write(buf, count), duration);
  }

  bool IsConnected() const { return connected_; }

  void Close() override {
    assert(connected_);
    connected_ = false;
    BasicSocket::Close();
  }

 private:
  std::atomic<bool> connected_ = false;
};

}  // namespace Cold::Net

#endif /* COLD_NET_TCPSOCKET */
