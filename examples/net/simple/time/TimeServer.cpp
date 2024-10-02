#include "cold/Cold.h"

using namespace Cold;

class TimeServer : public TcpServer {
 public:
  TimeServer(const IpAddress& addr, size_t poolSize = 0)
      : TcpServer(addr, poolSize) {}

  ~TimeServer() override = default;

  Task<> OnNewConnection(TcpSocket socket) override {
    INFO("New connection from: {}", socket.GetRemoteAddress().GetIpPort());
    auto value = static_cast<uint32_t>(time(nullptr));
    value = Endian::Host32ToNetwork32(value);
    co_await socket.WriteN(&value, sizeof(uint32_t));
    socket.Close();
  }
};

int main() {
  TimeServer server(IpAddress(8888), 4);
  server.Start();
  return 0;
}