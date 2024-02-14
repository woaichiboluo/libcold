#include "cold/coro/IoContextPool.h"
#include "cold/net/Acceptor.h"
#include "cold/net/TcpSocket.h"
#include "cold/net/UdpSocket.h"

using namespace Cold;

std::string MakeDayTimeString() {
  using namespace std;  // For time_t, time and ctime;
  time_t now = time(0);
  return ctime(&now);
}

class TcpDaytimeServer {
 public:
  TcpDaytimeServer(Base::IoContextPool& pool, const Net::IpAddress& addr)
      : acceptor_(pool, addr, true) {}

  void Start() {
    acceptor_.Listen();
    LOG_INFO(Base::GetMainLogger(), "TcpDaytimeServer run at:{}",
             acceptor_.GetLocalAddress().GetIpPort());
    acceptor_.GetIoContext()->CoSpawn(DoAccept());
  }

  Base::Task<> DoAccept() {
    while (true) {
      auto sock = co_await acceptor_.Accept();
      if (sock) sock->GetIoContext()->CoSpawn(DoWrite(std::move(sock.value())));
    }
  }

  Base::Task<> DoWrite(Net::TcpSocket socket) {
    auto str = MakeDayTimeString();
    co_await socket.Write(str.data(), str.size());
    socket.Close();
  }

 private:
  Net::Acceptor acceptor_;
};

class UdpDaytimeServer {
 public:
  UdpDaytimeServer(Base::IoContext& ioContext, const Net::IpAddress& addr)
      : udpSocket_(ioContext) {
    udpSocket_.Bind(addr);
  }

  void Start() {
    udpSocket_.GetIoContext()->CoSpawn(DoDaytime());
    LOG_INFO(Base::GetMainLogger(), "UdpDaytimeServer run at:{}",
             udpSocket_.GetLocalAddress().GetIpPort());
  }

  Base::Task<> DoDaytime() {
    auto logger = Base::GetMainLogger();
    while (true) {
      Net::IpAddress addr;
      char buf[1];
      auto readBytes = co_await udpSocket_.RecvFrom(buf, sizeof buf, addr);
      if (readBytes < 0) {
        LOG_ERROR(logger, "Recvfrom error. errno = {},reason = {}", errno,
                  Base::ThisThread::ErrorMsg());
        continue;
      }
      LOG_INFO(logger, "Recvfrom addr:{}", addr.GetIpPort());
      auto str = MakeDayTimeString();
      co_await udpSocket_.SendTo(str.data(), str.size(), addr);
    }
  }

 private:
  Net::UdpSocket udpSocket_;
};

int main() {
  Base::IoContextPool pool(4);
  Net::IpAddress addr(13);
  TcpDaytimeServer tcpServer(pool, addr);
  UdpDaytimeServer udpServer(*pool.GetNextIoContext(), addr);
  tcpServer.Start();
  udpServer.Start();
  pool.Start();
}