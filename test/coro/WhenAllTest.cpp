#include "cold/Cold.h"

using namespace Cold;

Task<std::unique_ptr<int>> Sleep(int sec) {
  co_await Sleep(std::chrono::seconds(sec));
  INFO("Sleep{}Sec", sec);
  co_return std::make_unique<int>(sec);
}

Task<> BasicWhenAll() {
  INFO("BasicWhenAll start.");
  auto result = co_await WhenAll(Sleep(2), Sleep(3), Sleep(1));
  assert(*std::get<0>(result) == 2);
  assert(*std::get<1>(result) == 3);
  assert(*std::get<2>(result) == 1);
  INFO("BasicWhenAll end.");
  auto& context = co_await ThisCoro::GetIoContext();
  context.Stop();
}

Task<> TestSerialWhenAny() {
  INFO("TestSerialWhenAny start.");
  co_await (Sleep(1) & Sleep(2) & Sleep(3));
  INFO("TestSerialWhenAny end.");
  auto& context = co_await ThisCoro::GetIoContext();
  context.CoSpawn(BasicWhenAll());
}

int main() {
  IoContext context;
  context.CoSpawn(TestSerialWhenAny());
  context.Start();
}