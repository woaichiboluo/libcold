#include <iostream>

#include "cold/coro/IoContext.h"
#include "cold/net/TcpSocket.h"

using namespace Cold;

Base::Task<> DoEcho(Net::TcpSocket tcpSocket) {
  Net::IpAddress remoteAddress(8888);
  auto ret = co_await tcpSocket.Connect(remoteAddress);
  if (ret != 0) {
    LOG_ERROR(Base::GetMainLogger(), "Connect failed. errno = {},reason = {}",
              errno, Base::ThisThread::ErrorMsg());
    tcpSocket.GetIoContext()->Stop();
    co_return;
  }
  LOG_INFO(Base::GetMainLogger(), "Connected. addr: {} <-> {}",
           tcpSocket.GetLocalAddress().GetIpPort(),
           tcpSocket.GetRemoteAddress().GetIpPort());
  while (true) {
    std::string str;
    std::getline(std::cin, str);
    if (str == "quit") {
      tcpSocket.Close();
      break;
    }
    co_await tcpSocket.Write(str.data(), str.size());
    char buf[4096];
    auto readBytes = co_await tcpSocket.Read(buf, sizeof buf);
    if (readBytes <= 0) {
      tcpSocket.Close();
      break;
    } else {
      assert(static_cast<size_t>(readBytes) == str.size());
      fmt::println("{}", std::string_view(buf, buf + readBytes));
    }
  }
  tcpSocket.GetIoContext()->Stop();
}

int main() {
  Base::IoContext ioContext;
  Net::TcpSocket tcpSocket(ioContext);
  ioContext.CoSpawn(DoEcho(std::move(tcpSocket)));
  ioContext.Start();
}