#ifndef NET_RPC_RPCCLIENT
#define NET_RPC_RPCCLIENT

#include <google/protobuf/service.h>
#include <google/protobuf/stubs/callback.h>

#include <concepts>

#include "cold/coro/IoService.h"
#include "cold/net/TcpClient.h"
#include "cold/net/TcpSocket.h"
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

  class StubCallAwaitable {
   public:
    using Call = std::function<void(google::protobuf::Closure* done)>;
    StubCallAwaitable(TcpSocket* socket, Call call)
        : socket_(socket), call_(std::move(call)) {}

    bool await_ready() const noexcept { return !socket_->IsConnected(); }

    void await_suspend(std::coroutine_handle<> handle) noexcept {
      call_(google::protobuf::NewCallback(
          this, &StubCallAwaitable::ResumeCoroutine, handle));
    }

    void ResumeCoroutine(std::coroutine_handle<> handle) {
      socket_->GetIoService().CoSpawn(
          [](std::coroutine_handle<> h) -> Base::Task<> {
            h.resume();
            co_return;
          }(handle));
    }

    int await_resume() noexcept {
      if (timeout_ || !socket_->IsConnected()) {
        errno = timeout_ ? ETIMEDOUT : ENOTCONN;
        return -1;
      }
      return 0;
    }

    bool GetTimeout() const { return timeout_; }
    void SetTimeout() { timeout_ = true; }

   private:
    TcpSocket* socket_;
    Call call_;
    bool timeout_ = false;
  };

 protected:
  template <typename RequestType, typename ResponseType,
            typename REP = typename std::chrono::seconds::rep,
            typename PERIOD = typename std::chrono::seconds::period>
  auto StubCall(
      void (StubType::*method)(google::protobuf::RpcController*,
                               const RequestType* m, ResponseType*,
                               google::protobuf::Closure*),
      google::protobuf::RpcController* controller, const RequestType* request,
      ResponseType* response,
      std::chrono::duration<REP, PERIOD> timeout = std::chrono::seconds(10)) {
    std::function<void(google::protobuf::Closure * done)> call =
        [this, method, controller, request,
         response](google::protobuf::Closure* done) {
          (stub_.*method)(controller, request, response, done);
        };

    return Base::IoTimeoutAwaitable(
        &socket_.GetIoService(), StubCallAwaitable(&socket_, std::move(call)),
        timeout);
  }

 protected:
  RpcChannel channel_;
  StubType stub_;
};

}  // namespace Cold::Net::Rpc

#endif /* NET_RPC_RPCCLIENT */
