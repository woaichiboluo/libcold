#include "cold/net/BasicSocket.h"

#include <unistd.h>

#include <cassert>

#include "cold/coro/IoContext.h"

using namespace Cold;

Net::BasicSocket::~BasicSocket() {
  if (fd_ >= 0) close(fd_);
}

Net::BasicSocket::BasicSocket(BasicSocket&& other)
    : ioContext_(other.ioContext_),
      fd_(other.fd_),
      localAddress_(other.localAddress_),
      remoteAddress_(other.remoteAddress_) {
  other.ioContext_ = nullptr;
  other.fd_ = -1;
}

Net::BasicSocket& Net::BasicSocket::operator=(BasicSocket&& other) {
  if (this == &other) return *this;
  if (fd_ >= 0) close(fd_);
  ioContext_ = other.ioContext_;
  fd_ = other.fd_;
  localAddress_ = other.localAddress_;
  remoteAddress_ = other.remoteAddress_;
  other.ioContext_ = nullptr;
  other.fd_ = -1;
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

void Net::BasicSocket::ShutDown() { shutdown(fd_, SHUT_WR); }

void Net::BasicSocket::Close() { shutdown(fd_, SHUT_RDWR); }