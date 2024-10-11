#include "cold/Cold.h"

using namespace Cold;

Task<std::unique_ptr<int>> Sleep(int sec) {
  co_await Sleep(std::chrono::seconds(sec));
  INFO("Sleep{}Sec", sec);
  co_return std::make_unique<int>(sec);
}

Task<> BasicWhenAny() {
  INFO("BasicWhenAny start.");
  auto result = co_await WhenAny(Sleep(2), Sleep(3), Sleep(1));
  assert(result.index() == 2);
  std::visit([](auto&& arg) { assert(*arg == 1); }, result);
  INFO("BasicWhenAny end.");
  auto& contet = co_await ThisCoro::GetIoContext();
  contet.Stop();
}

int main() {
  IoContext context;
  context.CoSpawn(BasicWhenAny());
  context.Start();
}