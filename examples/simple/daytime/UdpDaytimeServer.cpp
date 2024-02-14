#include "cold/net/UdpSocket.h"

using namespace Cold;

std::string MakeDayTimeString() {
  using namespace std;  // For time_t, time and ctime;
  time_t now = time(0);
  return ctime(&now);
}

class UdpDaytimeServer {
 public:
  UdpDaytimeServer(const Net::IpAddress& addr) : udpSocket_(ioContext_) {
    udpSocket_.Bind(addr);
  }

  void Start() {
    ioContext_.CoSpawn(DoDaytime());
    LOG_INFO(Base::GetMainLogger(), "UdpDaytimeServer run at:{}",
             udpSocket_.GetLocalAddress().GetIpPort());
    ioContext_.Start();
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
  Base::IoContext ioContext_;
  Net::UdpSocket udpSocket_;
};

int main() {
  Net::IpAddress addr(13);
  UdpDaytimeServer server(addr);
  server.Start();
}