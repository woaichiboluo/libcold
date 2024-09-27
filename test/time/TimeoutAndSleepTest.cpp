#include "cold/Cold.h"

using namespace Cold;
using namespace std::chrono;

Task<> InfinityJob(std::stop_token token) {
  co_await ThisCoro::SetStopToken(std::move(token));
  while (true) {
    INFO("InfinityJob");
    co_await Sleep(1s);
  }
}

Task<> Sleep5Sec() {
  INFO("before sleep 5sec.");
  co_await Sleep(5s);
  assert(false);
}

Task<> Timeout() {
  INFO("Timeout: will call stop5sec expect timeout.");
  auto timeout = co_await Timeout(Sleep5Sec(), 3s);
  INFO("Timeout: timeout:{}", timeout);
}

Task<int> Sleep3Sec() {
  INFO("before sleep 3sec.");
  co_await Sleep(3s);
  INFO("after sleep 3sec.");
  co_return 1;
}

Task<> NotTimeout() {
  INFO("NotTimeout: will call sleep3sec expect not timeout.");
  auto [timeout, value] = co_await Timeout(Sleep3Sec(), 5s);
  INFO("NotTimeout: timeout:{} value: {}", timeout, value);
}

Task<> DoSleep() {
  auto& context = co_await Cold::ThisCoro::GetIoContext();
  std::stop_source source;
  INFO("before sleep 3sec.");
  context.CoSpawn(InfinityJob(source.get_token()));
  co_await Sleep(3s);
  source.request_stop();
  INFO("after sleep 3sec.");
  context.CoSpawn(Timeout());
  context.CoSpawn(NotTimeout());
}

int main() {
  // LogManager::GetInstance().GetDefaultRaw()->SetLevel(LogLevel::TRACE);
  IoContext ioContext;
  ioContext.CoSpawn(DoSleep());
  ioContext.Start();
  return 0;
}