#ifndef COLD_NET_TCPSERVER
#define COLD_NET_TCPSERVER

#include "../io/IoContextPool.h"
#include "Acceptor.h"

namespace Cold {

class TcpServer {
 public:
  explicit TcpServer(const IpAddress& addr, size_t poolSize = 0,
                     const std::string& nameArg = "TcpServer")
      : localAddress_(addr),
        pool_(poolSize, nameArg),
        acceptor_(pool_.GetMainIoContext(), addr) {}
  virtual ~TcpServer() = default;

  TcpServer(const TcpServer&) = delete;
  TcpServer& operator=(const TcpServer&) = delete;

  void SetReusePort() { acceptor_.SetReusePort(); }

  IpAddress GetLocalAddress() const { return localAddress_; }

#ifdef COLD_ENABLE_SSL
  void EnableSSL(SSLContext& sslContext) {
    acceptor_.SetSSLContext(sslContext);
  }
#endif

  void Start() {
    assert(!started_);
    acceptor_.BindAndListen();
    acceptor_.GetIoContext().CoSpawn(DoAccept());
    started_ = true;
    pool_.Start();
  }

 protected:
  virtual Task<> DoAccept() {
    while (true) {
      TcpSocket socket = co_await acceptor_.Accept(pool_.GetNextIoContext());
      if (socket) {
        socket.GetIoContext().CoSpawn(OnNewConnection(std::move(socket)));
      }
    }
  }

  virtual Task<> OnNewConnection(TcpSocket socket) {
    INFO("TcpServer received new connection from: {}",
         socket.GetRemoteAddress().GetIpPort());
    socket.Close();
    co_return;
  }

  IpAddress localAddress_;
  IoContextPool pool_;
  Acceptor acceptor_;
  bool started_ = false;
};

}  // namespace Cold

#endif /* COLD_NET_TCPSERVER */
