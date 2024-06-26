#ifndef NET_HTTP_HTTPSERVER
#define NET_HTTP_HTTPSERVER

#include "cold/coro/IoServicePool.h"
#include "cold/net/Acceptor.h"
#include "cold/net/TcpSocket.h"
#include "cold/net/http/ServletContext.h"

namespace Cold::Net::Http {

class HttpServer {
 public:
  HttpServer(Net::IpAddress& addr, size_t poolSize = 0, bool reusePort = false)
      : pool_(poolSize), acceptor_(pool_.GetMainIoService(), addr, reusePort) {
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

  void SetHost(std::string host) { context_.host_ = host; }

  void SetDispatcherServlet(std::unique_ptr<DispatcherServlet> servlet) {
    assert(!started_);
    context_.dispatcher_ = std::move(servlet);
  }

  void SetDefaultServlet(std::unique_ptr<HttpServlet> servlet) {
    assert(!started_);
    context_.defaultServlet_ = std::move(servlet);
  }

  void AddServlet(std::string_view url, std::unique_ptr<HttpServlet> servlet) {
    assert(!started_);
    context_.dispatcher_->AddServlet(url, std::move(servlet));
  }

  void AddServlet(std::string_view url,
                  std::function<void(HttpRequest&, HttpResponse&)> handle) {
    assert(!started_);
    context_.dispatcher_->AddServlet(
        url, std::make_unique<FunctionHttpServlet>(handle));
  }

  void AddFilter(std::string_view url, std::unique_ptr<HttpFilter> filter) {
    assert(!started_);
    context_.dispatcher_->AddFilter(url, std::move(filter));
  }

  void Start() {
    acceptor_.Listen();
    acceptor_.GetIoService().CoSpawn(DoAccept());
    started_ = true;
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

  Base::Task<> DoHttp(Net::TcpSocket socket);

  bool IsStarted() const { return started_; }

  void AddErrorPageHandler(HttpStatus status,
                           std::function<void(HttpResponse&)> callback) {
    assert(!started_);
    errorPageHandler_[status] = std::move(callback);
  }

  void SetDefaultErrorPageHandler(std::function<void(HttpResponse&)> callback) {
    assert(!started_);
    defaultErrorPageHandler_ = std::move(callback);
  }

 private:
  Base::IoServicePool pool_;
  Net::Acceptor acceptor_;
  ServletContext context_;
  bool started_ = false;
  std::map<HttpStatus, std::function<void(HttpResponse&)>> errorPageHandler_;
  std::function<void(HttpResponse&)> defaultErrorPageHandler_;
};

}  // namespace Cold::Net::Http

#endif /* NET_HTTP_HTTPSERVER */
