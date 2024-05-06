#include <chrono>

#include "cold/coro/IoServicePool.h"
#include "cold/log/Logger.h"
#include "cold/net/Acceptor.h"
#include "cold/net/IpAddress.h"
#include "cold/net/TcpSocket.h"
#include "examples/simple/simplehttp/HttpRequest.h"
#include "examples/simple/simplehttp/HttpResponse.h"
#include "examples/simple/simplehttp/RequestParser.h"

using namespace Cold;

class HttpServer {
 public:
  HttpServer(size_t poolSize, Net::IpAddress& addr)
      : pool_(poolSize), acceptor_(pool_.GetMainIoService(), addr, true) {}

  ~HttpServer() = default;

  void Start() {
    acceptor_.Listen();
    acceptor_.GetIoService().CoSpawn(DoAccept());
    pool_.Start();
  }

  Base::Task<> DoAccept() {
    while (true) {
      auto socket = co_await acceptor_.Accept(pool_.GetNextIoService());
      if (socket) {
        socket.GetIoService().CoSpawn(DoHttp(std::move(socket)));
      }
    }
  }

  Base::Task<> DoHttp(Net::TcpSocket socket) {
    HttpRequest request;
    RequestParser parser;
    while (true) {
      char buf[8192];
      auto byteRead = co_await socket.ReadWithTimeout(buf, sizeof buf,
                                                      std::chrono::seconds(15));
      if (byteRead <= 0) {
        Base::INFO("Close Connection. fd:{} addr: {}", socket.NativeHandle(),
                   socket.GetRemoteAddress().GetIpPort());
        socket.Close();
        co_return;
      }
      auto state = parser.Parse({buf, buf + byteRead}, request);
      if (state == RequestParser::kKEEP) continue;
      HttpResponse response;
      HttpResponse::MakeResponse(state == RequestParser::kOK, request,
                                 response);
      auto ret = co_await response.SendRequest(socket);
      if (ret < 0 || state == RequestParser::kBAD ||
          request.GetVersion() == "HTTP/1.0") {
        Base::INFO("Close Connection. fd:{} addr: {}", socket.NativeHandle(),
                   socket.GetRemoteAddress().GetIpPort());
        socket.Close();
        co_return;
      }
      request = HttpRequest();
    }
  }

 private:
  Base::IoServicePool pool_;
  Net::Acceptor acceptor_;
};

int main() {
  Net::IpAddress addr(8080);
  HttpServer server(4, addr);
  Base::INFO("HttpServer run at: 8080");
  server.Start();
}