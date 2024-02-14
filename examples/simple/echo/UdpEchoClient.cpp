#include <iostream>

#include "cold/coro/IoContext.h"
#include "cold/net/UdpSocket.h"

using namespace Cold;

Base::Task<> DoEcho(Net::UdpSocket socket) {
  Net::IpAddress dest(6666);
  auto value = co_await socket.Connect(dest);
  assert(value == 0);
  while (true) {
    std::string str;
    std::getline(std::cin, str);
    if (str == "quit") {
      break;
    }
    co_await socket.Write(str.data(), str.size());
    char buf[4096];
    auto readBytes = co_await socket.Read(buf, sizeof buf);
    if (readBytes < 0) {
      LOG_ERROR(Base::GetMainLogger(), "Read error. errno = {},reason = {}",
                errno, Base::ThisThread::ErrorMsg());
      break;
    } else {
      assert(static_cast<size_t>(readBytes) == str.size());
      fmt::println("{}", std::string_view(buf, buf + readBytes));
    }
  }
  socket.GetIoContext()->Stop();
}

int main() {
  Base::IoContext ioContext;
  Net::UdpSocket udpSocket(ioContext);
  ioContext.CoSpawn(DoEcho(std::move(udpSocket)));
  ioContext.Start();
}