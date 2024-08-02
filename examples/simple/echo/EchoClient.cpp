#include "cold/coro/Io.h"
#include "cold/net/TcpClient.h"

using namespace Cold;

class EchoClient : public Net::TcpClient {
 public:
  EchoClient(Base::IoService& service) : TcpClient(service) {}
  ~EchoClient() = default;

  Base::Task<> OnConnect() override {
    Base::INFO("Connect Success. Server address:{}",
               socket_.GetRemoteAddress().GetIpPort());
    Base::AsyncIO io(socket_.GetIoService(), STDIN_FILENO);
    while (true) {
      char buf[4096];
      auto n = co_await io.AsyncRead(buf, sizeof buf);
      if (n <= 0) break;
      assert(n >= 1);
      std::string_view message(buf, static_cast<size_t>(n - 1));
      if (message == "quit") break;
      n = co_await socket_.WriteN(message.data(), message.size());
      if (n < 0) break;
      co_await io.AsyncWrite(buf, static_cast<size_t>(n));
      const char* newline = "\n";
      co_await io.AsyncWrite(newline, 1);
    }
    socket_.Close();
    socket_.GetIoService().Stop();
  }
};

int main() {
  Base::IoService ioService;
  EchoClient client(ioService);
  Net::IpAddress addr(8888);
  ioService.CoSpawn(client.Connect(addr));
  ioService.Start();
}