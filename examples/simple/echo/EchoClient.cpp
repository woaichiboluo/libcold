#include "cold/coro/Io.h"
#include "cold/coro/IoService.h"
#include "cold/net/IpAddress.h"
#include "cold/net/TcpSocket.h"

using namespace Cold;

class EchoClient {
 public:
  EchoClient(Base::IoService& service) : socket_(service) {}
  ~EchoClient() = default;
  EchoClient(const EchoClient&) = delete;
  EchoClient& operator=(const EchoClient&) = delete;

  Base::Task<> Connect(const Net::IpAddress& addr) {
    auto ret = co_await socket_.Connect(addr);
    if (ret < 0) {
      Base::ERROR("Connect failed");
      socket_.GetIoService().Stop();
      co_return;
    }
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

 private:
  Net::TcpSocket socket_;
};

int main() {
  Base::IoService ioService;
  EchoClient client(ioService);
  Net::IpAddress addr(8888);
  ioService.CoSpawn(client.Connect(addr));
  ioService.Start();
}