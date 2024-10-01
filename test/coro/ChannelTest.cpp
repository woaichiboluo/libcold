#include "cold/Cold.h"

using namespace Cold;

Channel<int> ch;

Task<> Sender() {
  for (int i = 0; i < 100000; ++i) {
    auto& context = co_await ThisCoro::GetIoContext();
    co_await context.RunInThisContext();
    co_await ch.Write(i);
    INFO("write value: {}", i);
  }
}

Task<> Reader(IoContextPool& pool) {
  for (int i = 0; i < 50000; ++i) {
    int value = co_await ch.Read();
    INFO("read value: {}", value);
    if (value == 99999) {
      auto& context = pool.GetMainIoContext();
      co_await context.RunInThisContext();
      pool.Stop();
    }
  }
}

int main() {
  IoContextPool pool(2);
  pool.GetNextIoContext().CoSpawn(Reader(pool));
  pool.GetNextIoContext().CoSpawn(Reader(pool));
  pool.GetMainIoContext().CoSpawn(Sender());
  pool.Start();
}