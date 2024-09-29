#include "cold/Cold.h"

using namespace Cold;
using namespace std::chrono;

IoContextPool g_pool(4);

Task<> Sleep(int sec, bool stop = false) {
  INFO("task start");
  co_await Sleep(seconds(sec));
  INFO("task end");
  if (stop) {
    g_pool.Stop();
  }
}

int main() {
  for (int i = 0; i < 4; ++i) {
    g_pool.GetNextIoContext().CoSpawn(Sleep(i + 1));
  }
  g_pool.GetMainIoContext().CoSpawn(Sleep(1, true));
  g_pool.Start();
  g_pool.GetMainIoContext().CoSpawn(Sleep(5, true));
  g_pool.Start();
  return 0;
}