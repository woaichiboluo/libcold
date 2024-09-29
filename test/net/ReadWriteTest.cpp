#include "cold/Cold.h"

using namespace std::chrono;
using namespace Cold;

Task<> DoEcho(TcpSocket sock) {
  char buf[1024];
  while (true) {
    // auto [timeout, n] =
    //     co_await sock.GetIoContext().Timeout(sock.Read(buf, sizeof(buf)),
    //     10s);
    auto [timeout, n] = co_await Timeout(sock.Read(buf, sizeof(buf)), 1s);
    if (timeout || n <= 0) {
      if (timeout) INFO("read timeout. close connection.");
      sock.Close();
      break;
    }
    INFO("read {} bytes from {}", n, sock.GetRemoteAddress().GetIpPort());
    if (co_await sock.Write(buf, static_cast<size_t>(n)) < 0) {
      sock.Close();
      break;
    }
  }
}

Task<> DoAccept(IoContext& ioContext) {
  Acceptor acceptor(ioContext, IpAddress(8888));
  acceptor.BindAndListen();
  while (true) {
    auto sock = co_await acceptor.Accept();
    if (sock) sock.GetIoContext().CoSpawn(DoEcho(std::move(sock)));
  }
}

int main() {
  LogManager::GetInstance().GetDefaultRaw()->SetLevel(LogLevel::TRACE);
  IoContext ioContext;
  ioContext.CoSpawn(DoAccept(ioContext));
  ioContext.Start();
  return 0;
}