#include "cold/net/Acceptor.h"

#include <fcntl.h>
#include <unistd.h>

#include "cold/log/Logger.h"
#include "cold/net/SocketOptions.h"
#include "cold/net/TcpSocket.h"

using namespace Cold;

Net::Acceptor::Acceptor(Base::IoService& service, const IpAddress& listenAddr,
                        bool reusePort)
    : BasicSocket(service,
                  socket(listenAddr.IsIpv4() ? AF_INET : AF_INET6,
                         SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0)),
      idleFd_(open("/dev/null", O_RDONLY | O_CLOEXEC)) {
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
  return Accept(*ioService_);
}

Base::Task<Net::TcpSocket> Net::Acceptor::Accept(Base::IoService& service) {
  auto [sockfd, addr] = co_await AcceptAwaitable(ioService_, fd_);
  if (sockfd < 0) {
    Base::ERROR("Accept Error. errno: {},reason: {}", errno,
                Base::ThisThread::ErrorMsg());
    if (errno == EMFILE) {
      close(idleFd_);
      idleFd_ = accept(fd_, nullptr, nullptr);
      close(idleFd_);
      idleFd_ = open("/dev/null", O_RDONLY | O_CLOEXEC);
    }
  }
  co_return Net::TcpSocket(service, localAddress_, addr, sockfd);
}

void Net::Acceptor::Listen() { listen(fd_, SOMAXCONN); }