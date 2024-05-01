#include "cold/coro/IoService.h"
#include "cold/net/IpAddress.h"
#include "cold/net/UdpSocket.h"

using namespace Cold;

class UdpEchoServer {
 public:
  UdpEchoServer(uint16_t port) : udpSocket_(service_) {
    Net::IpAddress addr(port);
    assert(udpSocket_.Bind(addr));
  }

  ~UdpEchoServer() = default;

  void Run() {
    service_.CoSpawn(DoRecv());
    service_.Start();
  }

  Base::Task<> DoRecv() {
    while (true) {
      Net::IpAddress addr;
      char buf[1024];
      auto ret = co_await udpSocket_.RecvFrom(buf, sizeof buf, addr);
      if (ret < 0) {
        Base::ERROR("Recvfrom Error errno = {}, reason = {}", errno,
                    Base::ThisThread::ErrorMsg());
      } else {
        Base::INFO("Recv: {} from: {}", std::string_view{buf, buf + ret},
                   addr.GetIpPort());
        co_await udpSocket_.SendTo(buf, static_cast<size_t>(ret), addr);
      }
    }
    co_return;
  }

 private:
  Base::IoService service_;
  Net::UdpSocket udpSocket_;
};

int main() {
  UdpEchoServer server(8888);
  Base::INFO("UdpEchoServer run at port 8888");
  server.Run();
}