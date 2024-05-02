#include "cold/coro/IoService.h"
#include "cold/net/TcpSocket.h"

using namespace Cold;

Base::Task<> Connect(Net::IpAddress addr, Net::TcpSocket socket) {
  auto ret = co_await socket.ConnectWithTimeout(addr, std::chrono::seconds(1));
  if (ret == 0) {
    fmt::println("Connect Success");
  } else {
    if (errno == ETIMEDOUT)
      fmt::println("Connect Timeout");
    else
      fmt::println("Error. errno = {}", errno);
  }
  socket.GetIoService().Stop();
}

int main(int argc, char** argv) {
  if (argc < 3) {
    fmt::println("Usage: {} <hostname> <service>", argv[0]);
    return 1;
  }
  auto addr = Net::IpAddress::Resolve(argv[1], argv[2]);
  if (!addr) {
    fmt::println("Error. errno = {}", errno);
  } else {
    Base::IoService service;
    Net::TcpSocket socket(service);
    service.CoSpawn(Connect(addr.value(), std::move(socket)));
    service.Start();
  }
}