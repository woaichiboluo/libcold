#include <iostream>

#include "cold/coro/IoContextPool.h"
#include "cold/net/Acceptor.h"
#include "cold/net/TcpSocket.h"

using namespace Cold;

Base::Task<> DoRead(Net::TcpSocket tcpSocket) {
  Net::IpAddress remoteAddress(13);
  auto ret = co_await tcpSocket.Connect(remoteAddress);
  if (ret != 0) {
    LOG_ERROR(Base::GetMainLogger(), "Connect failed. errno = {},reason = {}",
              errno, Base::ThisThread::ErrorMsg());
    tcpSocket.GetIoContext()->Stop();
    co_return;
  }
  for (;;) {
    char buf[256];
    auto readBytes = co_await tcpSocket.Read(buf, sizeof buf);
    if (readBytes <= 0) {
      tcpSocket.Close();
      break;
    } else {
      fmt::println("{}", std::string_view(buf, buf + readBytes));
    }
  }
  tcpSocket.GetIoContext()->Stop();
}

int main() {
  Base::IoContext ioContext;
  Net::TcpSocket tcpSocket(ioContext);
  ioContext.CoSpawn(DoRead(std::move(tcpSocket)));
  ioContext.Start();
}