#include "cold/net/rpc/RpcChannel.h"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>

#include <cassert>
#include <chrono>

#include "cold/net/TcpSocket.h"
#include "cold/net/rpc/RpcServer.h"
#include "cold/net/rpc/rpc.pb.h"

using namespace Cold;

std::atomic_int64_t Net::Rpc::RpcChannel::id_ = 0;

void Net::Rpc::RpcChannel::CallMethod(
    const google::protobuf::MethodDescriptor* method,
    google::protobuf::RpcController* controller,
    const google::protobuf::Message* request,
    google::protobuf::Message* response, google::protobuf::Closure* done) {
  auto rpcMessage = std::make_unique<RpcMessage>();
  rpcMessage->set_id(++id_);
  rpcMessage->set_service(method->service()->full_name());
  rpcMessage->set_method(method->name());
  rpcMessage->set_payload(request->SerializeAsString());
  socket_->GetIoService().CoSpawn(SendResponse(std::move(rpcMessage)));
  outstandings_.emplace(id_, OutstandingCall{response, done});
}

Base::Task<> Net::Rpc::RpcChannel::DoRpc() {
  assert(socket_);
  while (socket_->IsConnected()) {
    char buf[65535];
    auto readBytes = co_await socket_->ReadWithTimeout(
        buf, sizeof buf, std::chrono::seconds(10));
    if (readBytes <= 0) {
      socket_->Close();
      co_return;
    }
    auto [ok, message] =
        codec_.ParseMessage(buf, static_cast<size_t>(readBytes));
    if (!ok) continue;
    std::string messageStr(message);
    codec_.TakeMessage(message);
    if (forServer_) {
      co_await DoServerRpc(messageStr);
    } else {
      co_await DoClientRpc(messageStr);
    }
  }
}

Base::Task<> Net::Rpc::RpcChannel::DoServerRpc(std::string_view messageStr) {
  assert(socket_ && forServer_);
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
    google::protobuf::Message* response =
        service->GetResponsePrototype(methodDesc).New();
    service->CallMethod(
        methodDesc, nullptr, request.get(), response,
        google::protobuf::NewCallback(this, &RpcChannel::SendResponse, response,
                                      id_.load()));

  } while (0);
  if (error != ErrorCode::NO_ERROR) {
    badRpcMessage.set_error(error);
    badRpcMessage.set_id(rpcMessage.id());
    co_await Send(badRpcMessage);
  }
}

void Net::Rpc::RpcChannel::SendResponse(google::protobuf::Message* response,
                                        int64_t id) {
  if (!socket_->IsConnected()) return;
  std::unique_ptr<google::protobuf::Message> guard(response);
  auto message = std::make_unique<RpcMessage>();
  message->set_id(id);
  std::string data;
  if (response->SerializeToString(&data)) {
    message->set_payload(data);
  } else {
    message->set_error(ErrorCode::INTERNAL_SERVER_ERROR);
  }
  socket_->GetIoService().CoSpawn(SendResponse(std::move(message)));
}

Base::Task<> Net::Rpc::RpcChannel::SendResponse(
    std::unique_ptr<google::protobuf::Message> response) {
  if (!socket_->IsConnected()) co_return;
  co_await Send(*response);
}

Base::Task<> Net::Rpc::RpcChannel::Send(
    const google::protobuf::Message& message) {
  codec_.WriteMessageToBuffer(message);
  auto& buf = codec_.GetBuffer();
  auto writeBytes = co_await socket_->WriteNWithTimeout(
      buf.data(), buf.size(), std::chrono::seconds(10));
  if (writeBytes < 0 || static_cast<size_t>(writeBytes) != buf.size()) {
    socket_->Close();
  }
}

std::string_view ErrorCodeToString(Net::Rpc::ErrorCode error) {
  switch (error) {
    case Net::Rpc::ErrorCode::NO_ERROR:
      return "NO_ERROR";
    case Net::Rpc::ErrorCode::BAD_REQUEST:
      return "BAD_REQUEST";
    case Net::Rpc::ErrorCode::NO_SERVICE:
      return "NO_SERVICE";
    case Net::Rpc::ErrorCode::NO_METHOD:
      return "NO_METHOD";
    case Net::Rpc::ErrorCode::INTERNAL_SERVER_ERROR:
      return "INTERNAL_SERVER_ERROR";
    default:
      return "UNKNOWN";
  }
}

Base::Task<> Net::Rpc::RpcChannel::DoClientRpc(std::string_view messageStr) {
  assert(socket_ && !forServer_);
  RpcMessage rpcMessage;
  if (!rpcMessage.ParseFromString(messageStr)) {
    Base::WARN("Bad response from server,Close the connection");
    co_return;
  }
  if (rpcMessage.error() != ErrorCode::NO_ERROR) {
    Base::WARN("Request Error. Error Code : {}",
               ErrorCodeToString(rpcMessage.error()));
  }
  OutstandingCall call = {nullptr, nullptr};
  {
    auto it = outstandings_.find(rpcMessage.id());
    if (it != outstandings_.end()) {
      call = it->second;
      outstandings_.erase(it);
    }
  }
  if (call.response) {
    call.response->ParseFromString(rpcMessage.payload());
  }
  if (call.done) call.done->Run();
}