#define COLD_ENABLE_SSL
#include "cold/Cold.h"

using namespace Cold;

class EchoServer : public TcpServer {
 public:
  EchoServer(const IpAddress& addr, size_t poolSize = 0,
             const std::string& nameArg = "EchoServer")
      : TcpServer(addr, poolSize, nameArg) {}

  ~EchoServer() override = default;

  Task<> DoEcho(TcpSocket socket) {
    char buf[1024];
    while (true) {
      auto n = co_await socket.Read(buf, sizeof(buf));
      if (n <= 0) {
        break;
      }
      INFO("recv: {}", std::string_view(buf, static_cast<size_t>(n)));
      if (co_await socket.WriteN(buf, static_cast<size_t>(n)) != n) {
        break;
      }
    }
    socket.Close();
  }

  Task<> OnNewConnection(TcpSocket socket) override {
    co_await DoEcho(std::move(socket));
  }
};

int main(int argc, char* argv[]) {
  if (argc < 3) {
    fmt::println("Usage: {} <cert> <key>", argv[0]);
    return 1;
  }
  SSLContext sslContext;
  sslContext.LoadCert(argv[1], argv[2]);
  EchoServer server(IpAddress(8888));
  server.EnableSSL(sslContext);
  server.Start();
}