#ifndef COLD_NET_TCPSERVER
#define COLD_NET_TCPSERVER

#include "cold/coro/IoServicePool.h"
#include "cold/net/Acceptor.h"
#include "cold/net/IpAddress.h"
#include "cold/net/TcpSocket.h"

namespace Cold::Net {

class TcpServer {
 public:
  TcpServer(const Net::IpAddress& addr, size_t poolSize = 0,
            bool reusePort = false, bool enableSSL = false)
      : pool_(poolSize),
        acceptor_(pool_.GetMainIoService(), addr, reusePort, enableSSL) {}

  virtual ~TcpServer() = default;

  TcpServer(TcpServer const&) = delete;
  TcpServer& operator=(TcpServer const&) = delete;

  void Start() {
    if (started_) {
      Base::FATAL("TcpServer: Already started");
    }
    acceptor_.Listen();
    pool_.GetMainIoService().CoSpawn(DoAccept());
    started_ = true;
    pool_.Start();
  }

  bool IsStarted() const { return started_; }

 protected:
  virtual Base::Task<> DoAccept() {
    while (true) {
      auto socket = co_await acceptor_.Accept();
      if (socket) {
        socket.GetIoService().CoSpawn(OnConnect(std::move(socket)));
      }
    }
  }

  virtual Base::Task<> OnConnect(Net::TcpSocket socket) {
    Base::INFO("TcpServer: Connection accepted. addr: {}",
               socket.GetRemoteAddress().GetIpPort());
    co_return;
  }

 private:
  Base::IoServicePool pool_;
  Net::Acceptor acceptor_;
  bool started_ = false;
};

}  // namespace Cold::Net

#endif /* COLD_NET_TCPSERVER */
