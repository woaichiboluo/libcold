#include "cold/coro/IoService.h"
#include "cold/log/LogCommon.h"
#include "cold/log/Logger.h"
#include "cold/net/Acceptor.h"
#include "cold/net/TcpSocket.h"

using namespace Cold;

Base::Task<> DoAccept(Net::Acceptor& acceptor) {
  while (true) {
    auto socket = co_await acceptor.Accept();
    if (socket) {
      Base::INFO("New Connection. remote address: {}, local address: {}",
                 socket.GetRemoteAddress().GetIpPort(),
                 socket.GetLocalAddress().GetIpPort());
      socket.Close();
    }
  }
}

int main() {
  Base::LogManager::Instance().GetMainLoggerRaw()->SetLevel(
      Base::LogLevel::TRACE);
  Base::IoService ioService;
  Net::Acceptor acceptor(ioService, Net::IpAddress(8888), true);
  acceptor.Listen();
  ioService.CoSpawn(DoAccept(acceptor));
  ioService.Start();
}