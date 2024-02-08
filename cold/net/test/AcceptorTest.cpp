#include "cold/coro/IoContext.h"
#include "cold/net/Acceptor.h"
#include "cold/net/TcpSocket.h"

using namespace Cold;

Base::Task<> DoAccept(Net::Acceptor& acceptor) {
  while (true) {
    auto socket = co_await acceptor.Accept();
    if (socket) {
      LOG_INFO(Base::GetMainLogger(), "New Connection. address:{} address:{}",
               socket->GetRemoteAddress().GetIpPort(),
               socket->GetLocalAddress().GetIpPort());
      socket->Close();
    }
  }
}

int main() {
  Base::IoContext ioContext;
  Net::Acceptor acceptor(ioContext, Net::IpAddress(8888), true);
  acceptor.Listen();
  ioContext.CoSpawn(DoAccept(acceptor));
  ioContext.Start();
}