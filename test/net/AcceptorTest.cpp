#include <fcntl.h>

#include "cold/Cold.h"

using namespace Cold;

Task<> DoAccept(IoContext* ioContext) {
  Acceptor acceptor(*ioContext, IpAddress(8888));
  acceptor.Listen();
  while (true) {
    auto socket = co_await acceptor.Accept();
    INFO("New Connection from {}", socket.GetRemoteAddress().GetIpPort());
  }
}

int main() {
  LogManager::GetInstance().GetDefaultRaw()->SetLevel(LogLevel::TRACE);
  IoContext ioContext;
  ioContext.CoSpawn(DoAccept(&ioContext));
  ioContext.Start();
}