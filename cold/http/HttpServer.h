#ifndef COLD_HTTP_HTTPSERVER
#define COLD_HTTP_HTTPSERVER

#include "../net/TcpServer.h"
#include "HttpRequest.h"
#include "HttpRequestParser.h"
#include "ServletContext.h"

namespace Cold::Http {

class HttpServer : public TcpServer {
 public:
  HttpServer(const IpAddress& addr, uint16_t poolSize = 0)
      : TcpServer(addr, poolSize, "HttpServer") {}
  ~HttpServer() override = default;

  void SetHost(std::string host) { context_.SetHost(std::move(host)); }

  void SetDefaultServlet(std::unique_ptr<HttpServlet> servlet) {
    context_.SetDefaultServlet(std::move(servlet));
  }

  void SetDefaultDispatcher(std::unique_ptr<DisPatcherServlet> dispatcher) {
    context_.SetDispatcher(std::move(dispatcher));
  }

  void AddServlet(std::string url, std::unique_ptr<HttpServlet> servlet) {
    context_.GetDispatcher()->AddServlet(std::move(url), std::move(servlet));
  }

  void AddServlet(std::string url,
                  std::function<void(HttpRequest&, HttpResponse&)> func) {
    AddServlet(std::move(url),
               std::make_unique<FunctionServlet>(std::move(func)));
  }

  void AddFilter(std::string url, std::unique_ptr<HttpFilter> filter) {
    context_.GetDispatcher()->AddFilter(std::move(url), std::move(filter));
  }

 private:
  Task<> DoHttp(TcpSocket socket) {
    HttpRequestParser parser;
    RawHttpRequest rawRequest;
    std::string headersBuf;
    char buf[8192];
    while (true) {
      auto n = co_await socket.Read(buf, sizeof buf);
      if (n <= 0) {
        break;
      }
      auto success = parser.Parse(buf, static_cast<size_t>(n));
      HttpResponse response;
      if (!success) {
        response.SetHttpConnectionStatus(kClose);
        response.SetHttpStatusCode(k400);
      } else if (parser.HasRequest()) {
        rawRequest = parser.TakeRequest();
        HttpRequest request(rawRequest);
        request.SetSevlertContext(&context_);
        response.SetHeader("Server", "Cold-HTTP");
        response.SetHeader("Host", context_.GetHost());
        response.SetHttpVersion(request.GetVersion());
        response.SetHttpConnectionStatus(request.GetConnectionStatus());
        request.SetHttpResponse(&response);
        context_.ForwardTo(request.GetUrl(), request, response);
      } else {
        continue;
      }
      headersBuf.clear();
      response.MakeHeaders(headersBuf);
      if (co_await socket.WriteN(headersBuf.data(), headersBuf.size()) !=
          static_cast<ssize_t>(headersBuf.size())) {
        break;
      }
      if (!co_await response.SendBody(socket)) {
        break;
      }
      if (response.GetHttpConnectionStatus() != kKeepAlive) {
        break;
      }
    }

    socket.Close();
  }

  Task<> OnNewConnection(TcpSocket socket) override {
    co_await DoHttp(std::move(socket));
  }
  ServletContext context_;
};

}  // namespace Cold::Http

#endif /* COLD_HTTP_HTTPSERVER */
