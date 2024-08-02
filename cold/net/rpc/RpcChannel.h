#ifndef NET_RPC_RPCCHANNEL
#define NET_RPC_RPCCHANNEL

#include <google/protobuf/service.h>

#include <unordered_map>

#include "cold/net/TcpSocket.h"
#include "cold/net/rpc/RpcCodec.h"

namespace Cold::Net::Rpc {

class RpcChannel : public google::protobuf::RpcChannel {
 public:
  explicit RpcChannel(Net::TcpSocket& socket)
      : socket_(&socket), forServer_(false) {}

  RpcChannel(Net::TcpSocket& socket,
             const std::unordered_map<std::string, google::protobuf::Service*>*
                 services)
      : socket_(&socket), forServer_(true), services_(services) {}

  ~RpcChannel() override = default;

  void CallMethod(const google::protobuf::MethodDescriptor* method,
                  google::protobuf::RpcController* controller,
                  const google::protobuf::Message* request,
                  google::protobuf::Message* response,
                  google::protobuf::Closure* done) override;

  Base::Task<> DoRpc();

 private:
  Base::Task<> DoServerRpc(std::string_view messageStr);
  Base::Task<> DoClientRpc(std::string_view messageStr);

  // for server call method callback
  void SendResponse(google::protobuf::Message* response, int64_t id);

  // for client call method
  Base::Task<> SendResponse(
      std::unique_ptr<google::protobuf::Message> response);

  // send wrapped RpcMessage
  Base::Task<> Send(const google::protobuf::Message& message);

  Net::TcpSocket* socket_ = nullptr;
  bool forServer_ = false;
  RpcCodec codec_;

  static std::atomic_int64_t id_;

  struct OutstandingCall {
    google::protobuf::Message* response;
    google::protobuf::Closure* done;
  };

  std::unordered_map<int64_t, OutstandingCall> outstandings_;
  const std::unordered_map<std::string, google::protobuf::Service*>* services_ =
      nullptr;
};

}  // namespace Cold::Net::Rpc
#endif /* NET_RPC_RPCCHANNEL */
