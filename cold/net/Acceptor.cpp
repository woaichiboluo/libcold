#include "cold/net/Acceptor.h"

#include <fcntl.h>
#include <unistd.h>

#include "cold/coro/IoContextPool.h"
#include "cold/log/Logger.h"
#include "cold/net/SocketOptions.h"
#include "cold/net/TcpSocket.h"

using namespace Cold;

Net::Acceptor::Acceptor(Base::IoContext& ioContext, const IpAddress& listenAddr,
                        bool reusePort)
    : BasicSocket(ioContext,
                  socket(listenAddr.IsIpv4() ? AF_INET : AF_INET6,
                         SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0)),
      idleFd_(open("/dev/null", O_RDONLY | O_CLOEXEC)) {
  assert(idleFd_ >= 0);
  if (fd_ < 0) {
    LOG_FATAL(Base::GetMainLogger(),
              "Cannot create listen fd. errno = {}. reason = {}", errno,
              Base::ThisThread::ErrorMsg());
  }
  SetOption(SocketOptions::ReuseAddress(true));
  if (reusePort) {
    SetOption(SocketOptions::ReusePort(true));
  }
  if (!Bind(listenAddr)) {
    LOG_FATAL(Base::GetMainLogger(),
              "Cannot bind listen fd. errno = {}. reason = {}", errno,
              Base::ThisThread::ErrorMsg());
  }
}

Net::Acceptor::Acceptor(Base::IoContextPool& ioContextPool,
                        const IpAddress& listenAddr, bool reusePort)
    : Acceptor(*ioContextPool.GetNextIoContext(), listenAddr, reusePort) {
  pool_ = &ioContextPool;
}

Net::Acceptor::~Acceptor() { close(idleFd_); }

Net::Acceptor::Acceptor(Acceptor&& other)
    : BasicSocket(std::move(other)),
      idleFd_(other.idleFd_),
      pool_(other.pool_) {
  other.idleFd_ = -1;
  other.pool_ = nullptr;
}

Net::Acceptor& Net::Acceptor::operator=(Acceptor&& other) {
  if (this == &other) return *this;
  BasicSocket::operator=(std::move(other));
  if (idleFd_ >= 0) close(idleFd_);
  idleFd_ = other.idleFd_;
  pool_ = other.pool_;
  other.idleFd_ = -1;
  other.pool_ = nullptr;
  return *this;
}

void Net::Acceptor::Listen() { listen(fd_, SOMAXCONN); }

Base::Task<std::optional<Net::TcpSocket>> Net::Acceptor::Accept() {
  auto [sockfd, addr] = co_await AcceptAwaitable(ioContext_, fd_);
  if (sockfd >= 0) {
    auto context = pool_ ? pool_->GetNextIoContext() : ioContext_;
    co_return Net::TcpSocket(*context, localAddress_, addr, sockfd);
  } else {
    LOG_ERROR(Base::GetMainLogger(), "Accept Error. errno={},reason={}", errno,
              Base::ThisThread::ErrorMsg());
    if (errno == EMFILE) {
      close(idleFd_);
      idleFd_ = accept(fd_, nullptr, nullptr);
      close(idleFd_);
      idleFd_ = open("/dev/null", O_RDONLY | O_CLOEXEC);
    }
    co_return {};
  }
}