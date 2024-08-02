#include <chrono>

#include "cold/net/IpAddress.h"
#include "cold/net/TcpServer.h"

using namespace Cold;

class EchoServer : public Net::TcpServer {
 public:
  EchoServer(const Net::IpAddress& addr, size_t poolSize = 0,
             bool reusePort = false, bool enableSSL = false)
      : Net::TcpServer(addr, poolSize, reusePort, enableSSL) {}
  ~EchoServer() override = default;

  Base::Task<> OnConnect(Net::TcpSocket socket) override {
    co_await DoEcho(std::move(socket));
  }

  Base::Task<> DoEcho(Net::TcpSocket socket) {
    auto timeout = std::chrono::seconds(10);
    while (true) {
      char buf[4096];
      auto n = co_await socket.ReadWithTimeout(buf, sizeof(buf), timeout);
      if (n <= 0) {
        socket.Close();
        break;
      }
      n = co_await socket.WriteNWithTimeout(buf, static_cast<size_t>(n),
                                            timeout);
      if (n < 0) {
        socket.Close();
        break;
      }
    }
  }
};

int main() {
  Net::IpAddress addr(8888);
  EchoServer server(addr);
  Base::INFO("EchoServer run at {}", addr.GetIpPort());
  server.Start();
}