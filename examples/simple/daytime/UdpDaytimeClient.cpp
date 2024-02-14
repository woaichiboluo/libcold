#include <iostream>

#include "cold/coro/IoContext.h"
#include "cold/net/UdpSocket.h"

using namespace Cold;

Base::Task<> DoDaytime(Net::UdpSocket socket) {
  Net::IpAddress dest(13);
  auto value = co_await socket.Connect(dest);
  assert(value == 0);
  co_await socket.Write("x", 1);
  char buf[256];
  auto readBytes = co_await socket.Read(buf, sizeof buf);
  if (readBytes < 0) {
    LOG_ERROR(Base::GetMainLogger(), "Read error. errno = {},reason = {}",
              errno, Base::ThisThread::ErrorMsg());
  } else {
    fmt::println("{}", std::string_view(buf, buf + readBytes));
  }
  socket.GetIoContext()->Stop();
}

int main() {
  Base::IoContext ioContext;
  Net::UdpSocket udpSocket(ioContext);
  ioContext.CoSpawn(DoDaytime(std::move(udpSocket)));
  ioContext.Start();
}