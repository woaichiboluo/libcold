#include <cerrno>
#include <chrono>

#include "cold/coro/IoService.h"
#include "cold/net/Acceptor.h"
#include "cold/net/IpAddress.h"
#include "cold/net/TcpSocket.h"
#include "cold/net/UdpSocket.h"

using namespace Cold;

class UdpEchoServer {
 public:
  UdpEchoServer(Base::IoService& service, uint16_t port) : udpSocket_(service) {
    Net::IpAddress addr(port);
    assert(udpSocket_.Bind(addr));
  }

  ~UdpEchoServer() = default;

  void Start() { udpSocket_.GetIoService().CoSpawn(DoRecv()); }

  Base::Task<> DoRecv() {
    while (true) {
      Net::IpAddress addr;
      char buf[1024];
      auto ret = co_await udpSocket_.RecvFromWithTimeout(
          buf, sizeof buf, addr, std::chrono::seconds(3));
      if (ret < 0) {
        if (errno == ETIMEDOUT) {
          Base::ERROR("Recvfrom timeout.");
        } else {
          Base::ERROR("Recvfrom Error errno = {}, reason = {}", errno,
                      Base::ThisThread::ErrorMsg());
        }
      } else {
        Base::INFO("Recv: {} from: {}", std::string_view{buf, buf + ret},
                   addr.GetIpPort());
        co_await udpSocket_.SendToWithTimeout(buf, static_cast<size_t>(ret),
                                              addr, std::chrono::seconds(1));
      }
    }
    co_return;
  }

 private:
  Net::UdpSocket udpSocket_;
};

class TcpEchoServer {
 public:
  TcpEchoServer(Base::IoService& service, const Net::IpAddress& listenAddr)
      : acceptor_(service, listenAddr, true) {}

  ~TcpEchoServer() = default;

  void Start() {
    acceptor_.Listen();
    acceptor_.GetIoService().CoSpawn(DoAccept());
  }

  Base::Task<> DoAccept() {
    while (true) {
      auto sock = co_await acceptor_.Accept();
      if (sock) {
        sock.GetIoService().CoSpawn(DoEcho(std::move(sock)));
      }
    }
  }

  Base::Task<> DoEcho(Net::TcpSocket conn) {
    Base::INFO("New connection fd: {}, conn: {} <-> {} ", conn.NativeHandle(),
               conn.GetLocalAddress().GetIpPort(),
               conn.GetRemoteAddress().GetIpPort());
    while (true) {
      char buf[2048];
      auto readBytes = co_await conn.ReadWithTimeout(buf, sizeof buf,
                                                     std::chrono::seconds(3));
      if (readBytes == 0) {
        Base::INFO("Close connection fd: {}, conn: {} <-> {}",
                   conn.NativeHandle(), conn.GetLocalAddress().GetIpPort(),
                   conn.GetRemoteAddress().GetIpPort());
        conn.Close();
        break;
      } else if (readBytes > 0) {
        std::string_view view{buf, buf + readBytes};
        if (view.back() == '\n') view = view.substr(0, view.size() - 1);
        Base::INFO("Received:{}", view);
        co_await conn.WriteWithTimeout(buf, static_cast<size_t>(readBytes),
                                       std::chrono::seconds(3));
      } else {
        if (errno == ETIMEDOUT) {
          Base::ERROR("Read timeout. Close connection fd: {}, conn: {} <-> {}",
                      conn.NativeHandle(), conn.GetLocalAddress().GetIpPort(),
                      conn.GetRemoteAddress().GetIpPort());
        } else {
          Base::ERROR(
              "Connection error. errno: {}, reason: {}, fd: {}, conn: {} "
              "<-> {}",
              errno, Base::ThisThread::ErrorMsg(), conn.NativeHandle(),
              conn.GetLocalAddress().GetIpPort(),
              conn.GetRemoteAddress().GetIpPort());
        }
        conn.Close();
        break;
      }
    }
    co_return;
  }

 private:
  Net::Acceptor acceptor_;
};

int main() {
  Base::IoService service;
  UdpEchoServer udpServer(service, 8888);
  Net::IpAddress addr(8888);
  TcpEchoServer tcpServer(service, addr);
  Base::INFO("TcpEchoServer and UdpEchoServer run at port 8888");
  udpServer.Start();
  tcpServer.Start();
  service.Start();
}