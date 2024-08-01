#include "cold/net/rpc/RpcServer.h"

#include <google/protobuf/descriptor.h>
#include <google/protobuf/service.h>

#include "cold/net/rpc/RpcChannel.h"

using namespace Cold;

void Net::Rpc::RpcServer::AddService(Service* service) {
  const google::protobuf::ServiceDescriptor* desc = service->GetDescriptor();
  services_[desc->full_name()] = service;
}

Base::Task<> Net::Rpc::RpcServer::DoRpc(Net::TcpSocket socket) {
  RpcChannel channel(std::move(socket), &services_);
  co_await channel.DoRpc();
}