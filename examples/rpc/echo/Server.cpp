#include "cold/log/Logger.h"
#include "cold/net/rpc/RpcServer.h"
#include "echo.pb.h"

class EchoServiceImpl : public Echo::EchoService {
 public:
  EchoServiceImpl() = default;
  ~EchoServiceImpl() override = default;
  void DoEcho(::google::protobuf::RpcController* controller,
              const ::Echo::EchoRequest* request,
              ::Echo::EchoResponse* response,
              ::google::protobuf::Closure* done) override {
    Cold::Base::INFO("Call method DoEcho :{}", request->data());
    response->set_data(request->data());
    done->Run();
  }
};

int main() {
  Cold::Net::IpAddress addr(8888);
  Cold::Net::Rpc::RpcServer server(addr);
  EchoServiceImpl service;
  server.AddService(&service);
  server.Start();
}