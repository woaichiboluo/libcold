#ifndef COLD_NET_BASICSOCKET
#define COLD_NET_BASICSOCKET

#include "../io/IoContext.h"
#include "IpAddress.h"

namespace Cold {
// BasicSocket is a base class for all socket classes.
class BasicSocket {
 public:
  enum ShutdownState { kNotShutdown, kShutdownWrite, kShutdownAll };

  BasicSocket() = default;

  BasicSocket(IoContext& ioContext, bool ipv6, bool isSockStream)
      : ioContext_(&ioContext), isSockStream_(isSockStream) {
    auto sock = socket(ipv6 ? AF_INET6 : AF_INET,
                       (isSockStream ? SOCK_STREAM : SOCK_DGRAM) |
                           SOCK_NONBLOCK | SOCK_CLOEXEC,
                       0);
    if (sock < 0) {
      ERROR("create Socket error. type: {}. errno: {}.  reason: {}",
            isSockStream ? "stream" : "datagram", errno,
            ThisThread::ErrorMsg());
    }
    event_ = ioContext_->TakeIoEvent(sock);
    isSockStream_ = isSockStream;
  }

  // adopt a created socket
  BasicSocket(std::shared_ptr<Detail::IoEvent> event, bool isSockStream)
      : ioContext_(&event->GetIoContext()),
        event_(std::move(event)),
        isSockStream_(isSockStream) {
    event_->EnableReading(true);
  }

  virtual ~BasicSocket() {
    if (event_) {
      int fd = event_->GetFd();
      event_->ReturnIoEvent();
      close(fd);
    }
  }

  operator bool() const { return event_ != nullptr; }

  bool IsValid() const { return event_ != nullptr; }

  BasicSocket(const BasicSocket&) = delete;
  BasicSocket& operator=(const BasicSocket&) = delete;

  BasicSocket(BasicSocket&& other)
      : ioContext_(other.ioContext_),
        event_(other.event_),
        isSockStream_(other.isSockStream_),
        connected_(other.connected_.load()),
        shutdownState_(other.shutdownState_.load()),
        localAddress_(other.localAddress_),
        remoteAddress_(other.remoteAddress_) {
    other.ioContext_ = nullptr;
    other.event_ = nullptr;
    other.connected_ = false;
  }

  BasicSocket& operator=(BasicSocket&& other) {
    if (this == &other) return *this;
    if (event_) {
      event_->ReturnIoEvent();
      close(event_->GetFd());
    }
    ioContext_ = other.ioContext_;
    event_ = other.event_;
    isSockStream_ = other.isSockStream_;
    connected_ = other.connected_.load();
    shutdownState_ = other.shutdownState_.load();
    localAddress_ = other.localAddress_;
    remoteAddress_ = other.remoteAddress_;
    other.ioContext_ = nullptr;
    other.event_ = nullptr;
    other.connected_ = false;
    return *this;
  }

  IoContext& GetIoContext() const {
    assert(ioContext_);
    return *ioContext_;
  }

  IpAddress GetLocalAddress() const { return localAddress_; }
  IpAddress GetRemoteAddress() const { return remoteAddress_; }

  void SetLocalAddress(const IpAddress& addr) { localAddress_ = addr; }
  void SetRemoteAddress(const IpAddress& addr) { remoteAddress_ = addr; }

  bool Bind(IpAddress address) {
    assert(IsValid());
    auto ret = bind(event_->GetFd(), address.GetSockaddr(),
                    address.IsIpv4() ? sizeof(struct sockaddr_in)
                                     : sizeof(struct sockaddr_in6));
    if (ret == 0) {
      localAddress_ = address;
      return true;
    }
    return false;
  }

  void ShutDown() {
    assert(IsValid());
    shutdown(event_->GetFd(), SHUT_WR);
    shutdownState_ = kShutdownWrite;
  }

  void Close() {
    assert(IsValid());
    shutdown(event_->GetFd(), SHUT_RDWR);
    shutdownState_ = kShutdownAll;
    if (isSockStream_) connected_ = false;
  }

  bool IsConnected() const { return event_ && connected_; }

  bool CanReading() const {
    if (isSockStream_) return shutdownState_ != kShutdownAll && connected_;
    return shutdownState_ != kShutdownAll;
  }

  bool CanWriting() const {
    if (isSockStream_) return shutdownState_ == kNotShutdown && connected_;
    return shutdownState_ == kNotShutdown;
  }

  bool SetReuseAddr() {
    int opt = 1;
    return setsockopt(event_->GetFd(), SOL_SOCKET, SO_REUSEADDR, &opt,
                      sizeof(opt)) == 0;
  }

  bool SetReusePort() {
    int opt = 1;
    return setsockopt(event_->GetFd(), SOL_SOCKET, SO_REUSEPORT, &opt,
                      sizeof(opt)) == 0;
  }

  int GetNativeHandle() const {
    if (!event_) return -1;
    return event_->GetFd();
  }

 protected:
  IoContext* ioContext_ = nullptr;
  std::shared_ptr<Detail::IoEvent> event_;
  bool isSockStream_ = false;
  // for tcp socket connected_ symbolizes if the socket is connected
  // for udp socket connected_ symbolizes if the socket is bind
  std::atomic<bool> connected_ = false;
  std::atomic<ShutdownState> shutdownState_ = kNotShutdown;
  IpAddress localAddress_;
  IpAddress remoteAddress_;
};

}  // namespace Cold

#endif /* COLD_NET_BASICSOCKET */
