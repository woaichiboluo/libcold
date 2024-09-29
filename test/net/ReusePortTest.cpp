#include "cold/Cold.h"

using namespace Cold;

Task<> DoNothing(TcpSocket socket) {
  INFO("New connection addr: {}", socket.GetRemoteAddress().GetIpPort());
  socket.Close();
  co_return;
}

Task<> DoAccept() {
  auto& context = co_await ThisCoro::GetIoContext();
  Acceptor acceptor(context, IpAddress(8888));
  assert(acceptor.SetReusePort());
  acceptor.BindAndListen();
  while (true) {
    auto socket = co_await acceptor.Accept();
    if (socket) {
      co_await DoNothing(std::move(socket));
    }
  }
}

int main() {
  Cold::IoContextPool pool(4, "Reuse");
  pool.GetMainIoContext().CoSpawn(DoAccept());
  for (int i = 0; i < 4; ++i) {
    pool.GetNextIoContext().CoSpawn(DoAccept());
  }
  pool.Start();
  return 0;
}