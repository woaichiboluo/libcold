#include "cold/coro/IoContextPool.h"
#include "cold/net/Acceptor.h"
#include "cold/net/TcpSocket.h"
#include "cold/net/UdpSocket.h"

using namespace Cold;

class TcpAndUdpBothEchoServer {
 public:
  TcpAndUdpBothEchoServer(size_t threadNum, const Net::IpAddress& tcpAddr,
                          const Net::IpAddress& udpAddr)
      : pool_(threadNum),
        acceptor_(pool_, tcpAddr, true),
        udpSocket_(*pool_.GetNextIoContext(), udpAddr.IsIpv6()) {
    udpSocket_.Bind(udpAddr);
  }
  ~TcpAndUdpBothEchoServer() = default;

  void Start() {
    acceptor_.Listen();
    LOG_INFO(Base::GetMainLogger(), "Server run at:{}",
             acceptor_.GetLocalAddress().GetIpPort());
    acceptor_.GetIoContext()->CoSpawn(DoAccept());
    udpSocket_.GetIoContext()->CoSpawn(DoUdpEcho());
    pool_.Start();
  }

  Base::Task<> DoAccept() {
    while (true) {
      auto sock = co_await acceptor_.Accept();
      if (sock)
        sock->GetIoContext()->CoSpawn(DoTcpEcho(std::move(sock.value())));
    }
  }

  Base::Task<> DoTcpEcho(Net::TcpSocket socket) {
    auto logger = Base::GetMainLogger();
    LOG_INFO(logger, "New tcp connection. fd:{} addr: {} <-> {}",
             socket.NativeHandle(), socket.GetLocalAddress().GetIpPort(),
             socket.GetRemoteAddress().GetIpPort());
    while (true) {
      char buf[4096];
      auto readBytes = co_await socket.Read(buf, sizeof buf);
      if (readBytes == 0) {
        socket.Close();
        break;
      } else if (readBytes > 0) {
        std::string_view view{buf, buf + readBytes};
        if (view.back() == '\n') view = view.substr(0, view.size() - 1);
        co_await socket.Write(buf, static_cast<size_t>(readBytes));
      } else {
        LOG_ERROR(Base::GetMainLogger(), "error occurred errno:{},reason:{}",
                  errno, Base::ThisThread::ErrorMsg());
        socket.Close();
        break;
      }
    }
    LOG_INFO(logger, "Tcp connection close. fd:{} addr: {} <-> {}",
             socket.NativeHandle(), socket.GetLocalAddress().GetIpPort(),
             socket.GetRemoteAddress().GetIpPort());
  }

  Base::Task<> DoUdpEcho() {
    auto logger = Base::GetMainLogger();
    while (true) {
      Net::IpAddress addr;
      char buf[4096];
      auto readBytes = co_await udpSocket_.RecvFrom(buf, sizeof buf, addr);
      if (readBytes < 0) {
        LOG_ERROR(logger, "Recvfrom error. errno = {},reason = {}", errno,
                  Base::ThisThread::ErrorMsg());
        continue;
      }
      LOG_INFO(logger, "Udp recvfrom addr:{}", addr.GetIpPort());
      co_await udpSocket_.SendTo(buf, static_cast<size_t>(readBytes), addr);
    }
  }

 private:
  Base::IoContextPool pool_;
  Net::Acceptor acceptor_;
  Net::UdpSocket udpSocket_;
};

int main() {
  Net::IpAddress tcpAddr(8888), udpAddr(6666);
  TcpAndUdpBothEchoServer server(4, tcpAddr, udpAddr);
  server.Start();
}