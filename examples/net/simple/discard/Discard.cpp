#include "cold/Cold.h"

using namespace Cold;

class DisCard : public TcpServer {
 public:
  DisCard(const IpAddress& addr, size_t poolSize = 0)
      : TcpServer(addr, poolSize) {}

  ~DisCard() override = default;

  Task<> OnNewConnection(TcpSocket socket) override {
    INFO("New connection from: {}", socket.GetRemoteAddress().GetIpPort());
    socket.Close();
    co_return;
  }
};

int main() {
  DisCard server(IpAddress(8888), 4);
  server.Start();
  return 0;
}