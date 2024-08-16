#ifndef NET_HTTP_HTTPSERVER
#define NET_HTTP_HTTPSERVER

#include "cold/net/TcpServer.h"
#include "cold/net/http/ServletContext.h"

#ifdef COLD_NET_ENABLE_SSL
#include "cold/net/http/WebSocketServer.h"
#endif

namespace Cold::Net::Http {

class HttpServer : public TcpServer {
 public:
  HttpServer(const Net::IpAddress& addr, size_t poolSize = 0,
             bool reusePort = false, bool enableSSL = false)
      : TcpServer(addr, poolSize, reusePort, enableSSL) {
    defaultErrorPageHandler_ = [](HttpResponse& response) {
      auto body = std::make_unique<TextBody>();
      auto status = static_cast<int>(response.GetStatus());
      auto message = response.GetStatusMessage();
      body->SetContent(fmt::format(
          "<html><head><title>{} {}</title></head><body><center><h1>{} "
          "{}</h1></center><hr><center>Cold/1.0.0</center></body></html>",
          status, message, status, message));
      response.SetBody(std::move(body));
      response.SetHeader("Content-type", "text/html;charset=utf-8");
    };
  }

  ~HttpServer() = default;

  HttpServer(const HttpServer&) = delete;
  HttpServer& operator=(const HttpServer&) = delete;

  Base::Task<> OnConnect(Net::TcpSocket socket) override {
    co_await DoHttp(std::move(socket));
  }

  void SetHost(std::string host) { context_.host_ = host; }

  void SetDispatcherServlet(std::unique_ptr<DispatcherServlet> servlet) {
    assert(!IsStarted());
    context_.dispatcher_ = std::move(servlet);
  }

  void SetDefaultServlet(std::unique_ptr<HttpServlet> servlet) {
    assert(!IsStarted());
    context_.defaultServlet_ = std::move(servlet);
  }

  void AddServlet(std::string_view url, std::unique_ptr<HttpServlet> servlet) {
    assert(!IsStarted());
    context_.dispatcher_->AddServlet(url, std::move(servlet));
  }

  void AddServlet(std::string_view url,
                  std::function<void(HttpRequest&, HttpResponse&)> handle) {
    assert(!IsStarted());
    context_.dispatcher_->AddServlet(
        url, std::make_unique<FunctionHttpServlet>(handle));
  }

  void AddFilter(std::string_view url, std::unique_ptr<HttpFilter> filter) {
    assert(!IsStarted());
    context_.dispatcher_->AddFilter(url, std::move(filter));
  }

  Base::Task<> DoHttp(Net::TcpSocket socket);

  void AddErrorPageHandler(HttpStatus status,
                           std::function<void(HttpResponse&)> callback) {
    assert(!IsStarted());
    errorPageHandler_[status] = std::move(callback);
  }

  void SetDefaultErrorPageHandler(std::function<void(HttpResponse&)> callback) {
    assert(!IsStarted());
    defaultErrorPageHandler_ = std::move(callback);
  }

 private:
  ServletContext context_;
  std::map<HttpStatus, std::function<void(HttpResponse&)>> errorPageHandler_;
  std::function<void(HttpResponse&)> defaultErrorPageHandler_;

#ifdef COLD_NET_ENABLE_SSL
 public:
  void SetWebSocketServer(std::unique_ptr<WebSocketServer> wsServer) {
    assert(!IsStarted());
    wsSeerver_ = std::move(wsServer);
  }

 private:
  std::unique_ptr<WebSocketServer> wsSeerver_;
#endif
};

}  // namespace Cold::Net::Http

#endif /* NET_HTTP_HTTPSERVER */
