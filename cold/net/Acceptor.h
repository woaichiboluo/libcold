#ifndef COLD_NET_ACCEPTOR
#define COLD_NET_ACCEPTOR

#include "cold/coro/IoService.h"
#include "cold/net/BasicSocket.h"

namespace Cold::Net {
class TcpSocket;

class Acceptor : public BasicSocket {
 public:
  Acceptor(Base::IoService& service, const IpAddress& listenAddr,
           bool reusePort, bool enableSSL = false);

  ~Acceptor() override;

  Acceptor(Acceptor&&);
  Acceptor& operator=(Acceptor&&);

  void Listen();

  bool GetListened() const { return listened_; }

  Base::Task<TcpSocket> Accept();
  Base::Task<TcpSocket> Accept(Base::IoService& service);

 private:
  int idleFd_;
  bool listened_ = false;
  bool enableSSL_ = false;
};

}  // namespace Cold::Net

#endif /* COLD_NET_ACCEPTOR */
