#include "cold/Cold.h"

using namespace Cold;

Task<> ServerRun() {
  TcpServer server(IpAddress(8888));
  server.SetReusePort();
  server.Start();
  co_return;
}

int main() {
  IoContextPool pool(4, "Reuse");
  pool.GetMainIoContext().CoSpawn(ServerRun());
  for (int i = 0; i < 4; ++i) {
    pool.GetNextIoContext().CoSpawn(ServerRun());
  }
  pool.Start();
  return 0;
}