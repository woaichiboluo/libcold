#include "cold/Cold.h"

using namespace Cold;

class DaytimeServer : public TcpServer {
 public:
  DaytimeServer(const IpAddress& addr, size_t poolSize = 0)
      : TcpServer(addr, poolSize) {}

  ~DaytimeServer() override = default;

  Task<> OnNewConnection(TcpSocket socket) override {
    INFO("New connection from: {}", socket.GetRemoteAddress().GetIpPort());
    char buf[256];
    time_t now = time(0);
    ctime_r(&now, buf);
    std::string_view data(buf);
    co_await socket.WriteN(data.data(), data.size());
    socket.Close();
  }
};

int main() {
  DaytimeServer server(IpAddress(8888), 4);
  server.Start();
  return 0;
}