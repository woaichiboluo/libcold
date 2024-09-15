#include <fcntl.h>

#include "cold/Io.h"
#include "cold/Log.h"
#include "cold/Net.h"

using namespace Cold;

Task<> DoEcho(TcpSocket socket) {
  char buffer[1024];
  while (true) {
    auto n = co_await socket.Read(buffer, sizeof(buffer));
    if (n <= 0) {
      socket.Close();
      break;
    }
    co_await socket.Write(buffer, static_cast<size_t>(n));
  }
}

Task<> DoAccept(IoContext* ioContext) {
  Acceptor acceptor(*ioContext, IpAddress(8888));
  acceptor.Listen();
  while (true) {
    auto socket = co_await acceptor.Accept();
    if (socket) {
      INFO("Accept a new connection :{}",
           socket.GetRemoteAddress().GetIpPort());
      ioContext->CoSpawn(DoEcho(std::move(socket)));
    }
  }
}

int main() {
  LogManager::GetInstance().GetDefaultRaw()->SetLevel(LogLevel::TRACE);
  IoContext ioContext;
  ioContext.CoSpawn(DoAccept(&ioContext));
  ioContext.Start();
}