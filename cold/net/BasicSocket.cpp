#include "cold/net/BasicSocket.h"

#include <sys/signal.h>
#include <unistd.h>

#include <cassert>

using namespace Cold;

namespace {
class IgnoreSigPipe {
 public:
  IgnoreSigPipe() {
    ::signal(SIGPIPE, SIG_IGN);
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGPIPE);
    sigprocmask(SIG_BLOCK, &set, nullptr);
  }
};

IgnoreSigPipe __;
}  // namespace

Net::BasicSocket::~BasicSocket() {
  if (ssl_) {
#ifdef COLD_NET_ENABLE_SSL
    SSL_free(ssl_);
#endif
  }
  if (fd_ >= 0) {
    ioService_->StopListeningAll(fd_);
    close(fd_);
  }
}

Net::BasicSocket::BasicSocket(BasicSocket&& other)
    : ioService_(other.ioService_),
      fd_(other.fd_),
      localAddress_(other.localAddress_),
      remoteAddress_(other.remoteAddress_),
      connected_(other.connected_.load()),
      ssl_(other.ssl_) {
  other.ioService_ = nullptr;
  other.fd_ = -1;
  other.connected_ = false;
  other.ssl_ = nullptr;
}

Net::BasicSocket& Net::BasicSocket::operator=(BasicSocket&& other) {
  if (this == &other) return *this;
  if (fd_ >= 0) close(fd_);
  ioService_ = other.ioService_;
  fd_ = other.fd_;
  localAddress_ = other.localAddress_;
  remoteAddress_ = other.remoteAddress_;
  connected_ = other.connected_.load();
  ssl_ = other.ssl_;
  other.ioService_ = nullptr;
  other.fd_ = -1;
  other.connected_ = false;
  other.ssl_ = nullptr;
  return *this;
}

bool Net::BasicSocket::Bind(IpAddress address) {
  assert(IsValid());
  int ret = bind(fd_, address.GetSockaddr(),
                 address.IsIpv4() ? sizeof(struct sockaddr_in)
                                  : sizeof(struct sockaddr_in6));
  if (ret == 0) {
    localAddress_ = address;
    return true;
  }
  return false;
}

void Net::BasicSocket::ShutDown() {
#ifdef COLD_NET_ENABLE_SSL
  if (ssl_) SSL_shutdown(ssl_);
#endif
  shutdown(fd_, SHUT_WR);
}

void Net::BasicSocket::Close() {
#ifdef COLD_NET_ENABLE_SSL
  if (ssl_) SSL_shutdown(ssl_);
#endif
  shutdown(fd_, SHUT_RDWR);
}