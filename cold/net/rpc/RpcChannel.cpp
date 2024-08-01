#include "cold/net/rpc/RpcChannel.h"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

#include <cassert>
#include <chrono>

#include "cold/net/TcpSocket.h"
#include "cold/net/rpc/RpcServer.h"
#include "cold/net/rpc/rpc.pb.h"

using namespace Cold;

void Net::Rpc::RpcChannel::CallMethod(
    const google::protobuf::MethodDescriptor* method,
    google::protobuf::RpcController* controller,
    const google::protobuf::Message* request,
    google::protobuf::Message* response, google::protobuf::Closure* done) {
  auto rpcMessage = std::unique_ptr<RpcMessage>();
  rpcMessage->set_service(method->service()->full_name());
  rpcMessage->set_method(method->name());
  rpcMessage->set_payload(request->SerializeAsString());
  socket_.GetIoService().CoSpawn(SendResponse(std::move(rpcMessage)));
}

Base::Task<> Net::Rpc::RpcChannel::DoRpc() {
  if (forServer_) {
    co_await DoServerRpc();
  } else {
    co_await DoClientRpc();
  }
}

Base::Task<> Net::Rpc::RpcChannel::DoServerRpc() {
  assert(socket_ && forServer_);
  while (socket_.IsConnected()) {
    char buf[65535];
    auto readBytes = co_await socket_.ReadWithTimeout(buf, sizeof buf,
                                                      std::chrono::seconds(10));
    if (readBytes <= 0) {
      socket_.ShutDown();
      co_return;
    }
    auto [ok, message] =
        codec_.ParseMessage(buf, static_cast<size_t>(readBytes));
    if (!ok) continue;
    std::string messageStr(message);
    codec_.TakeMessage(message);
    ErrorCode error = ErrorCode::NO_ERROR;
    RpcMessage rpcMessage, badRpcMessage;
    if (!rpcMessage.ParseFromString(messageStr)) {
      error = ErrorCode::BAD_REQUEST;
    }
    do {
      if (error != ErrorCode::NO_ERROR) break;
      google::protobuf::Service* service = nullptr;
      {
        auto it = services_->find(rpcMessage.service());
        if (it != services_->end()) service = it->second;
      }
      if (!service) {
        error = ErrorCode::NO_SERVICE;
        break;
      }
      const google::protobuf::ServiceDescriptor* serviceDesc =
          service->GetDescriptor();
      const google::protobuf::MethodDescriptor* methodDesc =
          serviceDesc->FindMethodByName(rpcMessage.method());
      if (!methodDesc) {
        error = ErrorCode::NO_METHOD;
        break;
      }
      // find method
      auto request = std::unique_ptr<google::protobuf::Message>(
          service->GetRequestPrototype(methodDesc).New());
      if (!request->ParseFromString(rpcMessage.payload())) {
        error = ErrorCode::BAD_REQUEST;
        break;
      }
      auto response = service->GetResponsePrototype(methodDesc).New();
      // DO Callback
      // service->CallMethod(methodDesc, nullptr, request.get(), response,
      //                     );
    } while (0);
    if (error != ErrorCode::NO_ERROR) {
      badRpcMessage.set_error(error);
      co_await SendResponse(badRpcMessage);
    }
  }
}


Base::Task<> Net::Rpc::RpcChannel::SendResponse(
    std::unique_ptr<google::protobuf::Message> response) {
  codec_.WriteMessageToBuffer(*response);
  auto& buf = codec_.GetBuffer();
  auto writeBytes = co_await socket_.WriteNWithTimeout(
      buf.data(), buf.size(), std::chrono::seconds(10));
  if (writeBytes < 0 || static_cast<size_t>(writeBytes) != buf.size()) {
    socket_.ShutDown();
  }
}