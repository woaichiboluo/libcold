#include "cold/net/Acceptor.h"

#include <fcntl.h>
#include <unistd.h>

#include "cold/log/Logger.h"
#include "cold/net/SocketOptions.h"
#include "cold/net/TcpSocket.h"
#include "cold/thread/Thread.h"
#include "cold/util/Config.h"

#ifdef COLD_NET_ENABLE_SSL
#include "cold/net/ssl/SSLContext.h"
#include "cold/util/ScopeUtil.h"
#endif

using namespace Cold;

Net::Acceptor::Acceptor(Base::IoService& service, const IpAddress& listenAddr,
                        bool reusePort, bool enableSSL)
    : BasicSocket(service,
                  socket(listenAddr.IsIpv4() ? AF_INET : AF_INET6,
                         SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0),
                  false),
      idleFd_(open("/dev/null", O_RDONLY | O_CLOEXEC)),
      enableSSL_(enableSSL) {
  assert(idleFd_ >= 0);
  if (fd_ < 0) {
    Base::FATAL("Cannot create listen fd. errno: {}. reason: {}", errno,
                Base::ThisThread::ErrorMsg());
  }
  SetOption(SocketOptions::ReuseAddress(true));
  if (reusePort) {
    SetOption(SocketOptions::ReusePort(true));
  }
  if (!Bind(listenAddr)) {
    Base::FATAL("Cannot bind listen fd. errno: {}. reason: {}", errno,
                Base::ThisThread::ErrorMsg());
  }
#ifndef COLD_NET_ENABLE_SSL
  enableSSL_ = false;
#else
  if (enableSSL_ && !SSLContext::GetInstance().CertLoaded()) {
    Base::FATAL("SSL Cert not loaded.");
  }
#endif
}

Net::Acceptor::~Acceptor() { close(idleFd_); }

Net::Acceptor::Acceptor(Acceptor&& other)
    : BasicSocket(std::move(other)), idleFd_(other.idleFd_) {
  other.idleFd_ = -1;
}

Net::Acceptor& Net::Acceptor::operator=(Acceptor&& other) {
  if (this == &other) return *this;
  BasicSocket::operator=(std::move(other));
  if (idleFd_ >= 0) close(idleFd_);
  idleFd_ = other.idleFd_;
  other.idleFd_ = -1;
  return *this;
}

Base::Task<Net::TcpSocket> Net::Acceptor::Accept() {
  assert(listened_);
  return Accept(*ioService_);
}

#ifdef COLD_NET_ENABLE_SSL

Base::Task<SSL*> DoHandshake(Base::IoService* service, int& sockfd,
                             Net::IpAddress addr) {
  auto ssl = SSL_new(Net::SSLContext::GetInstance().GetContext());
  bool error = false;
  Base::ScopeGuard guard([&]() {
    if (error) {
      SSL_free(ssl);
      close(sockfd);
      sockfd = -1;
    }
  });
  if (ssl == nullptr) {
    Base::ERROR("SSL_new failed. errno: {}, reason: {}", errno,
                Base::ThisThread::ErrorMsg());
    error = true;
    co_return nullptr;
  }
  if (!SSL_set_fd(ssl, sockfd)) {
    error = true;
    co_return nullptr;
  }
  SSL_set_accept_state(ssl);
  while (true) {
    auto ret = co_await Net::IoTimeoutAwaitable(
        service, Net::HandshakeAwaitable(service, sockfd, ssl),
        std::chrono::seconds(
            Base::Config::GetGloablDefaultConfig().GetOrDefault<int>(
                "/ssl/server-handshake-timeout", 10)));
    if (ret == SSL_ERROR_NONE) break;
    if (ret != SSL_ERROR_WANT_READ && ret != SSL_ERROR_WANT_WRITE) {
      error = true;
      co_return nullptr;
    }
  }
  co_return ssl;
}

#endif

Base::Task<Net::TcpSocket> Net::Acceptor::Accept(Base::IoService& service) {
  assert(listened_);
  auto [sockfd, addr] = co_await AcceptAwaitable(ioService_, fd_);
#ifdef COLD_NET_ENABLE_SSL
  if (enableSSL_) {
    auto ssl = co_await DoHandshake(&service, sockfd, addr);
    co_return Net::TcpSocket(service, localAddress_, addr, sockfd, ssl);
  }
#endif
  if (sockfd < 0) {
    if (errno == EMFILE) {
      close(idleFd_);
      idleFd_ = accept(fd_, nullptr, nullptr);
      close(idleFd_);
      idleFd_ = open("/dev/null", O_RDONLY | O_CLOEXEC);
    } else {
      Base::ERROR("Accept Error. errno: {}, reason: {}", errno,
                  Base::ThisThread::ErrorMsg());
    }
  }
  co_return Net::TcpSocket(service, localAddress_, addr, sockfd);
}

void Net::Acceptor::Listen() {
  if (listen(fd_, SOMAXCONN) < 0) {
    Base::FATAL("listen error. errno = {}, reason = {}", errno,
                Base::ThisThread::ErrorMsg());
  }
  listened_ = true;
}
