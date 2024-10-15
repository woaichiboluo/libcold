#include "cold/Cold.h"

using namespace Cold;

class EchoClient {
 public:
  EchoClient(IoContext& context) : socket_(context) {}
  ~EchoClient() = default;

  Task<> ConnectOrStop(IpAddress addr) {
    if (!co_await socket_.Connect(addr)) {
      ERROR("connect failed. reason: {}", ThisThread::ErrorMsg());
      socket_.GetIoContext().Stop();
    } else {
      co_await DoEcho();
    }
  }

 private:
  Task<> DoEcho() {
    char buf[1024];
    char buf1[1024];
    while (socket_.CanReading() && socket_.CanWriting()) {
      AsyncIo io(socket_.GetIoContext(), STDIN_FILENO);
      auto n = co_await io.AsyncReadSome(buf, sizeof buf);
      if (n <= 0) {
        socket_.Close();
        break;
      }
      // skip the last '\n'
      if (co_await socket_.WriteN(buf, static_cast<size_t>(n - 1)) != n - 1) {
        socket_.Close();
        break;
      }
      if (co_await socket_.ReadN(buf1, static_cast<size_t>(n - 1)) != n - 1) {
        socket_.Close();
        break;
      }
      fmt::println("{}", std::string_view(buf1, static_cast<size_t>(n - 1)));
    }
  }

  TcpSocket socket_;
};

int main() {
  IoContext context;
  EchoClient client(context);
  context.CoSpawn(client.ConnectOrStop(IpAddress(8888)));
  context.Start();
}