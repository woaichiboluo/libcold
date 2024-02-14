#include "cold/net/UdpSocket.h"

using namespace Cold;

class UdpEchoServer {
 public:
  UdpEchoServer(const Net::IpAddress& addr) : udpSocket_(ioContext_) {
    udpSocket_.Bind(addr);
  }

  void Start() {
    ioContext_.CoSpawn(DoEcho());
    LOG_INFO(Base::GetMainLogger(), "UdpEchoServer run at:{}",
             udpSocket_.GetLocalAddress().GetIpPort());
    ioContext_.Start();
  }

  Base::Task<> DoEcho() {
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
      LOG_INFO(logger, "Recvfrom addr:{}", addr.GetIpPort());
      co_await udpSocket_.SendTo(buf, static_cast<size_t>(readBytes), addr);
    }
  }

 private:
  Base::IoContext ioContext_;
  Net::UdpSocket udpSocket_;
};

int main() {
  Net::IpAddress addr(6666);
  UdpEchoServer server(addr);
  server.Start();
}