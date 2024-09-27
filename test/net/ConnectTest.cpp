#include "cold/Cold.h"

using namespace Cold;

// connect to www.baidu.com:80
Task<> Connect1(IoContext& context) {
  TcpSocket socket(context);
  auto addr = IpAddress::Resolve("www.baidu.com", "80");
  assert(addr);
  INFO("Connect1: try to connect to {}", addr->GetIpPort());
  auto ret = co_await socket.Connect(*addr);
  INFO("Connect1: Connect result: {}", ret);
  INFO("Connect1: Local: {}, Peer: {}", socket.GetLocalAddress().GetIpPort(),
       socket.GetRemoteAddress().GetIpPort());
  co_return;
}

// connect to bad address
// nearly 120s timeout
Task<> Connect2(IoContext& context) {
  TcpSocket socket(context);
  auto addr = IpAddress::Resolve("www.baidu.com", "9999");
  INFO("Connect2: try to connect to {}", addr->GetIpPort());
  auto ret = co_await socket.Connect(*addr);
  INFO("Connect2: Connect result: {}. reason: {}", ret, ThisThread::ErrorMsg());
  context.Stop();
}

// connect to bad address but with timeout
Task<> Connect3(IoContext& context) {
  TcpSocket socket(context);
  auto addr = IpAddress::Resolve("www.baidu.com", "9999");
  INFO("Connect3: try to connect to {} with timeout 3s", addr->GetIpPort());
  using namespace std::chrono;
  auto [timeout, ret] = co_await Timeout(socket.Connect(*addr), 3s);
  INFO("Connect3: timeout: {}", timeout);
}

int main() {
  LogManager::GetInstance().GetDefaultRaw()->SetLevel(LogLevel::TRACE);
  IoContext context;
  context.CoSpawn(Connect1(context));
  context.CoSpawn(Connect2(context));
  context.CoSpawn(Connect3(context));
  context.Start();
}