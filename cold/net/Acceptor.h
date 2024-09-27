#ifndef COLD_NET_ACCEPTOR
#define COLD_NET_ACCEPTOR

#include <fcntl.h>

#include "TcpSocket.h"

namespace Cold {

class Acceptor : public TcpSocket {
 public:
  Acceptor(IoContext& context, const IpAddress& listenAddr)
      : TcpSocket(context, listenAddr.IsIpv6()) {
    listenAddr_ = listenAddr;
    idleFd_ = open("/dev/null", O_RDONLY | O_CLOEXEC);
    if (idleFd_ < 0) {
      FATAL("Cannot open /dev/null. errno: {}. reason: {}", errno,
            ThisThread::ErrorMsg());
    }
    SetReuseAddr();
    if (!Bind(listenAddr_)) {
      FATAL("Cannot bind. fd: {}. addr:{}. reason: {}", event_->GetFd(),
            listenAddr_.GetIpPort(), ThisThread::ErrorMsg());
    }
  }

  ~Acceptor() override {
    if (idleFd_ >= 0) {
      close(idleFd_);
    }
  }

  Acceptor(const Acceptor&) = delete;
  Acceptor& operator=(const Acceptor&) = delete;

  void EnableSSL() { sslEnabled_ = true; }

  void ReusePort(bool reuse) {
    int opt = reuse ? 1 : 0;
    if (setsockopt(event_->GetFd(), SOL_SOCKET, SO_REUSEPORT, &opt,
                   sizeof(opt)) < 0) {
      FATAL("setsockopt SO_REUSEPORT error. fd: {}. reason: {}",
            event_->GetFd(), ThisThread::ErrorMsg());
    }
  }

  void Listen() {
    if (listen(event_->GetFd(), SOMAXCONN) < 0) {
      FATAL("listen error. fd: {}. reason: {}", event_->GetFd(),
            ThisThread::ErrorMsg());
    }
    listened_ = true;
    event_->EnableReading(true);
  }

  bool IsListened() const { return listened_; }

  const IpAddress& GetListenAddr() const { return listenAddr_; }

  [[nodiscard]] Task<TcpSocket> Accept() {
    co_return co_await Accept(*ioContext_);
  }

  [[nodiscard]] Task<TcpSocket> Accept(IoContext& context) {
    assert(listened_);
    sockaddr_in6 addr;
    socklen_t addrLen = sizeof(addr);
    while (true) {
      auto sockfd = accept4(event_->GetFd(), reinterpret_cast<sockaddr*>(&addr),
                            &addrLen, SOCK_NONBLOCK | SOCK_CLOEXEC);
      if (sockfd >= 0) {
        auto ev = context.TakeIoEvent(sockfd);
        auto socket = TcpSocket(ev);
        socket.SetLocalAddress(localAddress_);
        socket.SetRemoteAddress(IpAddress(addr));
        co_return socket;
      } else {
        if (errno == EMFILE) {
          close(idleFd_);
          while ((idleFd_ = accept(event_->GetFd(), nullptr, nullptr)) < 0) {
            co_await Detail::ReadIoAwaitable(event_, false);
          }
          close(idleFd_);
          idleFd_ = open("/dev/null", O_RDONLY | O_CLOEXEC);
        } else if (errno == EAGAIN) {
          co_await Detail::ReadIoAwaitable(event_, false);
          continue;
        } else {
          ERROR("Accept Error. errno: {}. reason: {}", errno,
                ThisThread::ErrorMsg());
        }
        co_return TcpSocket();
      }
    }
  }

 private:
  int idleFd_;
  IpAddress listenAddr_;
  bool listened_ = false;
};

}  // namespace Cold

#endif /* COLD_NET_ACCEPTOR */
