#ifndef NET_RPC_RPCCHANNEL
#define NET_RPC_RPCCHANNEL

#include <google/protobuf/service.h>

#include <unordered_map>

#include "cold/net/TcpSocket.h"
#include "cold/net/rpc/RpcCodec.h"

namespace Cold::Net::Rpc {

class RpcChannel : public google::protobuf::RpcChannel {
 public:
  explicit RpcChannel(Net::TcpSocket socket)
      : socket_(std::move(socket)), forServer_(false) {}

  RpcChannel(Net::TcpSocket socket,
             const std::unordered_map<std::string, google::protobuf::Service*>*
                 services)
      : socket_(std::move(socket)), forServer_(true), services_(services) {}

  ~RpcChannel() = default;

  void CallMethod(const google::protobuf::MethodDescriptor* method,
                  google::protobuf::RpcController* controller,
                  const google::protobuf::Message* request,
                  google::protobuf::Message* response,
                  google::protobuf::Closure* done) override;

  Base::Task<> DoRpc();

 private:
  Base::Task<> DoServerRpc();
  Base::Task<> DoClientRpc();

  Base::Task<> SendResponse(const google::protobuf::Message& response);
  Base::Task<> SendResponse(
      std::unique_ptr<google::protobuf::Message> response);

  Net::TcpSocket socket_;
  bool forServer_;
  RpcCodec codec_;
  const std::unordered_map<std::string, google::protobuf::Service*>* services_;
};

}  // namespace Cold::Net::Rpc
#endif /* NET_RPC_RPCCHANNEL */
