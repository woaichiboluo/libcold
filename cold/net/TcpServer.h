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
  void EnableSSL(SSLContext& sslContext) { sslContext_ = &sslContext; }
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
#ifndef COLD_ENABLE_SSL
        socket.GetIoContext().CoSpawn(OnNewConnection(std::move(socket)));
#else
        if (sslContext_) {
          socket.GetIoContext().CoSpawn(
              HandshakeOrClose(std::move(socket), sslContext_));
        } else {
          socket.GetIoContext().CoSpawn(OnNewConnection(std::move(socket)));
        }
#endif
      }
    }
  }

  virtual Task<> OnNewConnection(TcpSocket socket) {
    INFO("TcpServer received new connection from: {}",
         socket.GetRemoteAddress().GetIpPort());
    socket.Close();
    co_return;
  }

#ifdef COLD_ENABLE_SSL
  virtual Task<> HandshakeOrClose(TcpSocket socket, SSLContext* sslContext) {
    auto ssl = co_await socket.Handshake(socket.event_, *sslContext, true);
    if (ssl) {
      socket.sslEnabled_ = true;
      socket.sslHodler_ = SSLHolder(ssl);
    } else {
      socket.Close();
    }
    co_await OnNewConnection(std::move(socket));
  }

#endif

  IpAddress localAddress_;
  IoContextPool pool_;
  Acceptor acceptor_;
  bool started_ = false;
#ifdef COLD_ENABLE_SSL
  SSLContext* sslContext_;
#endif
};

}  // namespace Cold

#endif /* COLD_NET_TCPSERVER */
