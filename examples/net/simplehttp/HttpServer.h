#ifndef EXAMPLES_NET_SIMPLEHTTP_HTTPSERVER
#define EXAMPLES_NET_SIMPLEHTTP_HTTPSERVER

#include "HttpResponse.h"
#include "RequestParser.h"
#include "cold/Cold.h"

class HttpServer : public Cold::TcpServer {
 public:
  HttpServer(const Cold::IpAddress& address, size_t poolSize = 0)
      : Cold::TcpServer(address, poolSize, "HttpServer") {}
  ~HttpServer() override = default;

 private:
  Cold::Task<> DoHttp(Cold::TcpSocket socket) {
    RequestParser parser;
    HttpRequest request;
    HttpResponse response;
    while (true) {
      char buf[16 * 1024];
      auto n = co_await socket.Read(buf, sizeof(buf));
      if (n <= 0) {
        break;
      }

      auto state = parser.Parse({buf, static_cast<size_t>(n)}, request);
      if (state == RequestParser::kKEEP) continue;
      HttpResponse::MakeResponse(state == RequestParser::kOK, request,
                                 response);
      auto headers = response.GetHeaders();
      n = co_await socket.WriteN(headers.data(), headers.size());
      if (n != static_cast<ssize_t>(headers.size())) {
        break;
      }
      if (response.HasBody()) {
        auto [data, size] = response.GetMmapAddr();
        n = co_await socket.WriteN(data, size);
        if (n != static_cast<ssize_t>(size)) {
          break;
        }
      }
      auto keepAlive = request.GetHeaders().find("Connection");
      if (state == RequestParser::kBAD || request.GetVersion() != "HTTP/1.1") {
        break;
      }
      if (keepAlive == request.GetHeaders().end() ||
          keepAlive->second != "keep-alive") {
        break;
      }
      request.Reset();
      response.Reset();
    }
    socket.Close();
  }

  Cold::Task<> OnNewConnection(Cold::TcpSocket socket) override {
    co_await DoHttp(std::move(socket));
  }
};

#endif /* EXAMPLES_NET_SIMPLEHTTP_HTTPSERVER */
