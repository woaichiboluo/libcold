#include "cold/Time.h"

using namespace Cold;
using namespace std::chrono;

Task<> DoTimeout(IoContext& ioContext);
Task<> DoTimeout1(IoContext& ioContext);

Task<> DoSleep(IoContext& ioContext) {
  INFO("before sleep. set sleep 3sec.");
  co_await Sleep(ioContext, 3s);
  INFO("after sleep 3sec.");
  ioContext.CoSpawn(DoTimeout(ioContext));
}

Task<> TestTimeout(IoContext& ioContext) {
  INFO("do with timeout job.");
  co_await Sleep(ioContext, 5s);
  // this should not be executed
  assert(false);
}

Task<> TestTimeout1(IoContext& ioContext) {
  INFO("do with timeout job.");
  co_await Sleep(ioContext, 5s);
  INFO("do with timeout job complete.");
}

// timeout
Task<> DoTimeout(IoContext& ioContext) {
  INFO("Begin test timeout (should be timeout)");
  auto value = co_await ioContext.Timeout(TestTimeout(ioContext), 3s);
  INFO("with timeout job return. timeout: {}", value);
  ioContext.CoSpawn(DoTimeout1(ioContext));
}

// not timeout
Task<> DoTimeout1(IoContext& ioContext) {
  INFO("Begin test timeout (should be not timeout)");
  auto value = co_await ioContext.Timeout(TestTimeout1(ioContext), 6s);
  INFO("with timeout job return. timeout: {}", value);
  ioContext.Stop();
}

int main() {
  //   LogManager::GetInstance().GetDefaultRaw()->SetLevel(LogLevel::TRACE);
  IoContext ioContext;
  ioContext.CoSpawn(DoSleep(ioContext));
  ioContext.Start();
  return 0;
}