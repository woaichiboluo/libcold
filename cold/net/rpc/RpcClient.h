#ifndef NET_RPC_RPCCLIENT
#define NET_RPC_RPCCLIENT

#include <google/protobuf/service.h>
#include <google/protobuf/stubs/callback.h>

#include <concepts>
#include <coroutine>
#include <type_traits>

#include "cold/coro/IoService.h"
#include "cold/net/TcpClient.h"
#include "cold/net/rpc/RpcChannel.h"

namespace Cold::Net::Rpc {

template <typename T>
concept ProtobufService = std::derived_from<T, google::protobuf::Service>;
// supported co_await stub_.method()
template <ProtobufService StubType>
class RpcClient : public Net::TcpClient {
 public:
  RpcClient(Base::IoService& service, bool enableSSL = false, bool ipv6 = false)
      : TcpClient(service, enableSSL, ipv6),
        channel_(socket_),
        stub_(&channel_) {}
  ~RpcClient() override = default;

  template <typename ResponseType>
  class StubCallAwaitable {
   public:
    using Call = std::function<void(google::protobuf::Closure* done)>;
    StubCallAwaitable(Base::IoService* service, Call call,
                      ResponseType* response)
        : service_(service), call_(std::move(call)), response_(response) {}

    bool await_ready() const noexcept { return false; }

    void await_suspend(std::coroutine_handle<> handle) noexcept {
      call_(google::protobuf::NewCallback(
          this, &StubCallAwaitable::ResumeCoroutine, handle));
    }

    void ResumeCoroutine(std::coroutine_handle<> handle) {
      service_->CoSpawn([](std::coroutine_handle<> h) -> Base::Task<> {
        h.resume();
        co_return;
      }(handle));
    }

    ResponseType* await_resume() noexcept { return response_; }

   private:
    Base::IoService* service_;
    Call call_;
    ResponseType* response_;
  };

 protected:
  template <typename RequestType, typename ResponseType>
  auto StubCall(void (StubType::*method)(google::protobuf::RpcController*,
                                         const RequestType* m, ResponseType*,
                                         google::protobuf::Closure*),
                google::protobuf::RpcController* controller,
                const RequestType* request, ResponseType* response) {
    std::function<void(google::protobuf::Closure * done)> call =
        [this, method, controller, request,
         response](google::protobuf::Closure* done) {
          (stub_.*method)(controller, request, response, done);
        };
    return StubCallAwaitable<ResponseType>(&socket_.GetIoService(),
                                           std::move(call), response);
  }

 protected:
  RpcChannel channel_;
  StubType stub_;
};

}  // namespace Cold::Net::Rpc

#endif /* NET_RPC_RPCCLIENT */
