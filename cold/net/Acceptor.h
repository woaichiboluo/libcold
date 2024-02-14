#ifndef COLD_NET_ACCEPTOR
#define COLD_NET_ACCEPTOR

#include "cold/coro/Task.h"
#include "cold/net/BasicSocket.h"

namespace Cold::Base {
class IoContextPool;
}

namespace Cold::Net {
class TcpSocket;

class Acceptor : public BasicSocket {
 public:
  Acceptor(Base::IoContext& ioContext, const IpAddress& listenAddr,
           bool reusePort);
  Acceptor(Base::IoContextPool& ioContextPool, const IpAddress& listenAddr,
           bool reusePort);
  ~Acceptor() override;

  Acceptor(Acceptor&&);
  Acceptor& operator=(Acceptor&&);

  void Listen();
  Base::Task<std::optional<TcpSocket>> Accept();
  template <typename PERIOD, typename REP>
  auto AcceptWithTimeout(std::chrono::duration<PERIOD, REP> duration) {
    return IoTimeoutAwaitable(Accept(), duration);
  }
  void SetIoContextPool(Base::IoContextPool* pool) { pool_ = pool; }

 private:
  int idleFd_;
  Base::IoContextPool* pool_;
};

}  // namespace Cold::Net

#endif /* COLD_NET_ACCEPTOR */
