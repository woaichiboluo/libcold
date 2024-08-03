#include "cold/net/IpAddress.h"
#include "cold/net/rpc/RpcClient.h"
#include "echo.pb.h"

using namespace Cold;

class EchoRpcClient : public Net::Rpc::RpcClient<Echo::EchoService::Stub> {
 public:
  EchoRpcClient(Base::IoService& service, bool enableSSL = false,
                bool ipv6 = false)
      : RpcClient(service, enableSSL, ipv6) {}
  ~EchoRpcClient() override = default;

  Base::Task<> OnConnect() override {
    Base::INFO("Connect Success. Server address:{}",
               GetSocket().GetRemoteAddress().GetIpPort());
    co_await DoEchoRpc();
  }

  Base::Task<> DoEchoRpc() {
    Base::AsyncIO io(socket_.GetIoService(), STDIN_FILENO);
    socket_.GetIoService().CoSpawn(channel_.DoRpc());
    while (true) {
      char buf[4096];
      auto n = co_await io.AsyncRead(buf, sizeof buf);
      if (n <= 0) break;
      assert(n >= 1);
      std::string_view message(buf, static_cast<size_t>(n - 1));
      if (message == "quit") break;
      Echo::EchoRequest request;
      request.set_data(message);
      auto response = std::make_unique<Echo::EchoResponse>();
      co_await StubCall(&Echo::EchoService::Stub::DoEcho, nullptr, &request,
                        response.get());
      Base::INFO("{}", response->data());
    }
    socket_.Close();
    socket_.GetIoService().Stop();
    co_return;
  }
};

int main() {
  Base::IoService service;
  EchoRpcClient client(service);
  service.CoSpawn(client.Connect(Net::IpAddress(8888)));
  service.Start();
}