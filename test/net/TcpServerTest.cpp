#include "cold/Cold.h"

using namespace Cold;
using namespace std::chrono;

class EchoServer : public TcpServer {
 public:
  explicit EchoServer(const IpAddress& addr, size_t poolSize = 0)
      : TcpServer(addr, poolSize, "EchoServer") {}
  ~EchoServer() override = default;

  Task<> DoEcho(TcpSocket socket) {
    while (true) {
      char buf[1024];
      auto [timeout, n] = co_await Timeout(socket.Read(buf, sizeof buf), 10s);
      if (timeout || n <= 0) {
        socket.Close();
        break;
      }
      INFO("EchoServer: received data: {}", n);
      if (co_await socket.WriteN(buf, static_cast<size_t>(n)) != n) {
        socket.Close();
        break;
      }
    }
  }

  Task<> OnNewConnection(TcpSocket socket) override {
    socket.GetIoContext().CoSpawn(DoEcho(std::move(socket)));
    co_return;
  }
};

int main() {
  EchoServer server(IpAddress(8888), 4);
  INFO("EchoServer run at : {}", server.GetLocalAddress().GetIpPort());
  server.Start();
}