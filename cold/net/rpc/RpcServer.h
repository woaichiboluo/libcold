#ifndef NET_RPC_RPCSERVER
#define NET_RPC_RPCSERVER

#include "cold/net/TcpServer.h"
#include "cold/net/TcpSocket.h"

namespace google::protobuf {
class Service;
}

namespace Cold::Net::Rpc {

class RpcServer : public TcpServer {
  using Service = google::protobuf::Service;

 public:
  RpcServer(const Net::IpAddress& addr, size_t poolSize = 0,
            bool reusePort = false, bool enableSSL = false)
      : TcpServer(addr, poolSize, reusePort, enableSSL) {}

  ~RpcServer() override = default;

  RpcServer(RpcServer const&) = delete;
  RpcServer& operator=(RpcServer const&) = delete;

  Base::Task<> OnConnect(Net::TcpSocket socket) override {
    co_await DoRpc(std::move(socket));
  }

  void AddService(Service* service);
  Base::Task<> DoRpc(Net::TcpSocket socket);

 private:
  std::unordered_map<std::string, Service*> services_;
};

}  // namespace Cold::Net::Rpc

#endif /* NET_RPC_RPCSERVER */
