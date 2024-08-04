#ifndef COLD_NET_TCPCLIENT
#define COLD_NET_TCPCLIENT

#include "cold/net/TcpSocket.h"

namespace Cold::Net {

class TcpClient {
 public:
  TcpClient(Base::IoService& service, bool enableSSL = false, bool ipv6 = false)
      : socket_(service, enableSSL, ipv6) {}

  virtual ~TcpClient() = default;

  TcpClient(const TcpClient&) = delete;
  TcpClient& operator=(const TcpClient&) = delete;

  TcpClient(TcpClient&&) = default;
  TcpClient& operator=(TcpClient&&) = default;

  Base::Task<> Connect(IpAddress addr) {
    auto ret = co_await socket_.Connect(addr);
    if (ret < 0) {
      co_await OnConnectFailed();
      co_return;
    }
#ifdef COLD_NET_ENABLE_SSL
    if (socket_.IsEnableSSL()) {
      bool ok = co_await socket_.DoHandshake();
      if (!ok) {
        co_await OnConnectFailed();
        co_return;
      }
    }
#endif
    co_await OnConnect();
  }

 protected:
  TcpSocket& GetSocket() { return socket_; }

  virtual Base::Task<> OnConnect() {
    Base::INFO("Connect Success. Server address:{}",
               socket_.GetRemoteAddress().GetIpPort());
    socket_.Close();
    co_return;
  }

  virtual Base::Task<> OnConnectFailed() {
    Base::ERROR("Connect Failed. reason: {}", Base::ThisThread::ErrorMsg());
    socket_.GetIoService().Stop();
    co_return;
  }

  virtual Base::Task<> OnSSLHandshakeFailed() {
    Base::ERROR("SSL Handshake Failed.");
    socket_.Close();
    socket_.GetIoService().Stop();
    co_return;
  }

  TcpSocket socket_;
};

}  // namespace Cold::Net

#endif /* COLD_NET_TCPCLIENT */
