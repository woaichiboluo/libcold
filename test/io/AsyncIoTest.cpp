#include "cold/Cold.h"

using namespace Cold;

Task<> TestAsyncIo() {
  auto& context = co_await ThisCoro::GetIoContext();
  AsyncIo forStdin(context, STDIN_FILENO);
  AsyncIo forStdout(context, STDOUT_FILENO);
  char buf[256] = {};
  while (true) {
    auto ret = co_await forStdin.AsyncReadSome(buf, sizeof buf);
    co_await forStdout.AsyncWriteSome(buf, static_cast<size_t>(ret));
  }
}

Task<> InfinityJob() {
  while (true) {
    INFO("Do InfinityJob");
    using namespace std::chrono_literals;
    co_await Sleep(3s);
  }
}

int main() {
  IoContext context;
  context.CoSpawn(TestAsyncIo());
  context.CoSpawn(InfinityJob());
  context.Start();
}