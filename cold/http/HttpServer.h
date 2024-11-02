#ifndef COLD_HTTP_HTTPSERVER
#define COLD_HTTP_HTTPSERVER

#include "../net/TcpServer.h"
#include "HttpRequest.h"
#include "HttpRequestParser.h"
#include "ServletContext.h"

#ifdef COLD_ENABLE_SSL
#include "websocket/WebsocketServer.h"
#endif

namespace Cold::Http {

class HttpServer : public TcpServer {
 public:
  HttpServer(const IpAddress& addr, uint16_t poolSize = 0)
      : TcpServer(addr, poolSize, "HttpServer") {}
  ~HttpServer() override = default;

  void SetHost(std::string host) {
    assert(!started_);
    context_.SetHost(std::move(host));
  }

  void SetDefaultServlet(std::unique_ptr<HttpServlet> servlet) {
    assert(!started_);
    context_.SetDefaultServlet(std::move(servlet));
  }

  void SetDefaultDispatcher(std::unique_ptr<DisPatcherServlet> dispatcher) {
    assert(!started_);
    context_.SetDispatcher(std::move(dispatcher));
  }

  void AddServlet(std::string url, std::shared_ptr<HttpServlet> servlet) {
    assert(!started_);
    context_.GetDispatcher()->AddServlet(std::move(url), std::move(servlet));
  }

  void AddServlet(std::string url,
                  std::function<void(HttpRequest&, HttpResponse&)> func) {
    assert(!started_);
    AddServlet(std::move(url),
               std::make_shared<FunctionServlet>(std::move(func)));
  }

  void AddFilter(std::string url, std::shared_ptr<HttpFilter> filter) {
    assert(!started_);
    context_.GetDispatcher()->AddFilter(std::move(url), std::move(filter));
  }

#ifdef COLD_ENABLE_SSL
  void AddWsServer(std::unique_ptr<WebSocketServer> wsServer) {
    assert(!started_);
    auto url = wsServer->GetUrl();
    wsRouter_.AddRoute(std::move(url), std::move(wsServer));
  }
#endif

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
#ifdef COLD_ENABLE_SSL
        if (WebSocketServer::CheckWhetherUpgradeRequest(rawRequest)) {
          auto ws = wsRouter_.MatchOne(rawRequest.GetUrl());
          if (ws) {
            co_await ws->OnReceivedUpgradeRequest(rawRequest,
                                                  std::move(socket));
            co_return;
          }
        }
#endif
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
#ifdef COLD_ENABLE_SSL
  Router<WebSocketServer> wsRouter_;
#endif
};

}  // namespace Cold::Http

#endif /* COLD_HTTP_HTTPSERVER */
