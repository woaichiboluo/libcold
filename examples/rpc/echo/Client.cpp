#include <cassert>
#include <memory>

#include "cold/coro/Task.h"
#include "cold/net/IpAddress.h"
#include "cold/net/rpc/RpcClient.h"
#include "echo.pb.h"

using namespace Cold;

// class EchoRpcClient : public Net::TcpClient {
//  public:
//   EchoRpcClient(Base::IoService& service, bool enableSSL = false,
//                 bool ipv6 = false)
//       : TcpClient(service, enableSSL, ipv6),
//         channel_(std::make_shared<Net::Rpc::RpcChannel>(socket_)),
//         stub_(channel_.get()) {}
//   ~EchoRpcClient() = default;

//   Base::Task<> OnConnect() override {
//     Base::INFO("Connect Success. Server address:{}",
//                socket_.GetRemoteAddress().GetIpPort());
//     co_await DoEcho();
//   }

//   Base::Task<> DoEcho() { co_return; }

//   void DoEcho(const std::string& data) {
//     Echo::EchoRequest request;
//     request.set_data(data);
//     auto response = new Echo::EchoResponse();
//     stub_.DoEcho(nullptr, &request, response,
//                  google::protobuf::NewCallback(
//                      this, &EchoRpcClient::RecvMessage, response));
//   }

//   void RecvMessage(Echo::EchoResponse* response) {
//     Base::INFO("recv message: {}", response->data());
//   }

//  private:
//   std::shared_ptr<Net::Rpc::RpcChannel> channel_;
//   Echo::EchoService::Stub stub_;
// };

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
      auto res = co_await StubCall(&Echo::EchoService::Stub::DoEcho, nullptr,
                                   &request, response.get());
      assert(response.get() == res);
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