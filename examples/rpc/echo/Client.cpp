#include "cold/net/rpc/RpcChannel.h"
#include "echo.pb.h"

using namespace Cold;

class EchoRpcClient {
 public:
  explicit EchoRpcClient(Base::IoService& service)
      : socket_(service),
        channel_(std::make_shared<Net::Rpc::RpcChannel>()),
        stub_(channel_.get()) {}

  ~EchoRpcClient() = default;

  Base::Task<bool> Connect(const Net::IpAddress& addr) {
    auto& ioService = socket_.GetIoService();
    auto ret = co_await socket_.Connect(addr);
    if (ret < 0) {
      Base::ERROR("connect failed");
      co_return false;
    }
    channel_->Reset(std::move(socket_));
    ioService.CoSpawn(channel_->DoRpc());
    co_return true;
  }

  void DoEcho(const std::string& data) {
    Echo::EchoRequest request;
    request.set_data(data);
    auto response = new Echo::EchoResponse();
    stub_.DoEcho(nullptr, &request, response,
                 google::protobuf::NewCallback(
                     this, &EchoRpcClient::RecvMessage, response));
  }

  void RecvMessage(Echo::EchoResponse* response) {
    Base::INFO("recv message: {}", response->data());
  }

 private:
  Net::TcpSocket socket_;
  std::shared_ptr<Net::Rpc::RpcChannel> channel_;
  Echo::EchoService::Stub stub_;
};

Base::Task<> DoRpc(EchoRpcClient& client) {
  if (!co_await client.Connect(Net::IpAddress(8888))) co_return;
  client.DoEcho("hello world");
  // client.DoEcho("hello another world");
}

int main() {
  Base::IoService service;
  EchoRpcClient client(service);
  service.CoSpawn(DoRpc(client));
  service.Start();
}