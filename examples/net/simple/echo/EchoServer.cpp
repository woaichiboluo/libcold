#include "cold/Cold.h"

using namespace Cold;

class EchoServer : public TcpServer {
 public:
  EchoServer(const IpAddress& addr, size_t poolSize = 0)
      : TcpServer(addr, poolSize) {}

  ~EchoServer() override = default;

 private:
  Task<> DoEcho(TcpSocket socket) {
    while (true) {
      char buf[1024];
      using namespace std::chrono_literals;
      auto [timeout, n] = co_await Timeout(socket.Read(buf, sizeof buf), 15s);
      if (timeout || n <= 0) {
        socket.Close();
        co_return;
      }
      if (co_await socket.WriteN(buf, static_cast<size_t>(n)) != n) {
        socket.Close();
        co_return;
      }
    }
  }

  Task<> OnNewConnection(TcpSocket socket) override {
    INFO("New connection from: {}", socket.GetRemoteAddress().GetIpPort());
    socket.GetIoContext().CoSpawn(DoEcho(std::move(socket)));
    co_return;
  }
};

int main() {
  EchoServer server(IpAddress(8888), 4);
  server.Start();
  return 0;
}