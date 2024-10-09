
#include "Socks4Tunnel.h"

using namespace Cold;

class Socks4a : public TcpServer {
 public:
  Socks4a(const IpAddress& localAddr, size_t poolsize = 0)
      : TcpServer(localAddr, poolsize, "Socks4a") {}
  ~Socks4a() override = default;

 private:
  static std::atomic<int> tunnelId_;
  Task<> OnNewConnection(TcpSocket socket) override {
    INFO("new connection from {}", socket.GetRemoteAddress().GetIpPort());
    auto tunnel =
        std::make_shared<Socks4aTunnel>(std::move(socket), ++tunnelId_);
    tunnel->SwapData();
    co_return;
  }
};

std::atomic<int> Socks4a::tunnelId_ = 0;

int main() {
  Socks4a server(IpAddress(8888));
  server.Start();
}