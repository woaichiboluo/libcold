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

  void Listen() {
    if (listen(event_->GetFd(), SOMAXCONN) < 0) {
      FATAL("listen error. fd: {}. reason: {}", event_->GetFd(),
            ThisThread::ErrorMsg());
    }
    listened_ = true;
    event_->EnableReadingET();
  }

  bool IsListened() const { return listened_; }

  const IpAddress& GetListenAddr() const { return listenAddr_; }

  Task<TcpSocket> Accept() { co_return co_await Accept(*ioContext_); }

  Task<TcpSocket> Accept(IoContext& context) {
    assert(listened_);
    sockaddr_in6 addr;
    socklen_t addrLen = sizeof(addr);
    auto connfd = co_await detail::AcceptAwaitable(
        event_, reinterpret_cast<struct sockaddr*>(&addr), &addrLen);
    if (connfd >= 0) {
      auto ev = context.TakeIoEvent(connfd);
      auto socket = TcpSocket(ev);
      socket.SetLocalAddress(localAddress_);
      socket.SetRemoteAddress(IpAddress(addr));
      co_return socket;
    } else {
      if (errno == EMFILE) {
        close(idleFd_);
        idleFd_ = accept(event_->GetFd(), nullptr, nullptr);
        close(idleFd_);
        idleFd_ = open("/dev/null", O_RDONLY | O_CLOEXEC);
      } else {
        ERROR("Accept Error. errno: {}. reason: {}", errno,
              ThisThread::ErrorMsg());
      }
      co_return TcpSocket();
    }
  }

 private:
  int idleFd_;
  IpAddress listenAddr_;
  bool listened_ = false;
};

}  // namespace Cold

#endif /* COLD_NET_ACCEPTOR */
