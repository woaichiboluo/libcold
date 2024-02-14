#ifndef COLD_NET_SOCKETOPTIONS
#define COLD_NET_SOCKETOPTIONS

#include <linux/tcp.h>
#include <netinet/in.h>
#include <sys/socket.h>

namespace Cold::Net::SocketOptions {

struct KeepAlive {
  explicit KeepAlive(bool open = false) : value(open ? 1 : 0) {}

  constexpr static int level = SOL_SOCKET;
  constexpr static int optName = SO_KEEPALIVE;
  int value;
  socklen_t len = sizeof(int);
};

struct Linger {
  Linger(bool open = false, int lingerTime = 0) {
    value.l_onoff = open ? 1 : 0;
    value.l_linger = lingerTime;
  }

  constexpr static int level = SOL_SOCKET;
  constexpr static int optName = SO_LINGER;
  struct linger value {};
  socklen_t len = sizeof(struct linger);
};

struct TcpNoDelay {
  explicit TcpNoDelay(bool open = false) : value(open ? 1 : 0) {}

  constexpr static int level = IPPROTO_TCP;
  constexpr static int optName = TCP_NODELAY;
  int value;
  socklen_t len = sizeof(int);
};

struct ReuseAddress {
  explicit ReuseAddress(bool open = false) : value(open ? 1 : 0) {}

  constexpr static int level = SOL_SOCKET;
  constexpr static int optName = SO_REUSEADDR;
  int value;
  socklen_t len = sizeof(int);
};

struct ReusePort {
  explicit ReusePort(bool open = false) : value(open ? 1 : 0) {}

  constexpr static int level = SOL_SOCKET;
  constexpr static int optName = SO_REUSEPORT;
  int value;
  socklen_t len = sizeof(int);
};

struct SockError {
  constexpr static int level = SOL_SOCKET;
  constexpr static int optName = SO_ERROR;
  int value = 0;
  socklen_t len = sizeof(int);
};

};  // namespace Cold::Net::SocketOptions

#endif /* COLD_NET_SOCKETOPTIONS */
