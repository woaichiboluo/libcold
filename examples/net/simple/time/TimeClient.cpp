#include "cold/Cold.h"

using namespace Cold;

class TimeClient {
 public:
  TimeClient(IoContext& context) : socket_(context) {}
  ~TimeClient() = default;

  Task<> ConnectOrStop(IpAddress addr) {
    if (!co_await socket_.Connect(addr)) {
      ERROR("connect failed. reason: {}", ThisThread::ErrorMsg());
      socket_.GetIoContext().Stop();
    } else {
      co_await DoTime();
    }
  }

 private:
  Task<> DoTime() {
    uint32_t value;
    if (co_await socket_.ReadN(&value, sizeof(uint32_t)) != sizeof(uint32_t)) {
      socket_.Close();
      co_return;
    }
    value = Endian::Network32ToHost32(value);
    fmt::println("server time: {}", value);
    socket_.Close();
    socket_.GetIoContext().Stop();
  }

  TcpSocket socket_;
};

int main() {
  IoContext context;
  TimeClient client(context);
  context.CoSpawn(client.ConnectOrStop(IpAddress(8888)));
  context.Start();
}