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
  }

  ~Acceptor() override {
    if (idleFd_ >= 0) {
      close(idleFd_);
    }
  }

  Acceptor(const Acceptor&) = delete;
  Acceptor& operator=(const Acceptor&) = delete;

#ifdef COLD_ENABLE_SSL
  // acceptor use SetSSLContext to enable ssl
  void SetSSLContext(SSLContext& sslContext) {
    sslContext_ = &sslContext;
    if (!sslContext_->IsCertLoaded()) {
      FATAL("SSL cert not loaded.");
    }
  }
#endif

  void BindAndListen() {
    if (!Bind(listenAddr_)) {
      FATAL("Cannot bind. fd: {}. addr:{}. reason: {}", event_->GetFd(),
            listenAddr_.GetIpPort(), ThisThread::ErrorMsg());
    }
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
      co_await ioContext_->RunInThisContext();
      auto sockfd = accept4(event_->GetFd(), reinterpret_cast<sockaddr*>(&addr),
                            &addrLen, SOCK_NONBLOCK | SOCK_CLOEXEC);
      if (sockfd >= 0) {
        co_await context.RunInThisContext();
        auto ev = context.TakeIoEvent(sockfd);
        TcpSocket socket;
#ifdef COLD_ENABLE_SSL
        if (sslContext_) {
          auto ssl = co_await Handshake(ev, *sslContext_, true);
          if (!ssl) {
            ev->ReturnIoEvent();
            close(sockfd);
            co_return socket;
          }
          socket = TcpSocket(ev, ssl);
        } else {
          socket = TcpSocket(ev);
        }
#else
        socket = TcpSocket(ev);
#endif
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

#ifdef COLD_ENABLE_SSL
  SSLContext* sslContext_ = nullptr;
#endif
};

}  // namespace Cold

#endif /* COLD_NET_ACCEPTOR */
