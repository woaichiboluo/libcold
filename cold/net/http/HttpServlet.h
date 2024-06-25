#ifndef NET_HTTP_HTTPSERVLET
#define NET_HTTP_HTTPSERVLET

#include "cold/net/http/HttpRequest.h"
#include "cold/net/http/HttpResponse.h"

namespace Cold::Net::Http {

class HttpRequest;
class HttpResponse;

class HttpServlet {
 public:
  HttpServlet() = default;
  virtual ~HttpServlet() = default;

  HttpServlet(const HttpServlet&) = delete;
  HttpServlet& operator=(const HttpServlet&) = delete;

  virtual void Handle(HttpRequest& request, HttpResponse& response) = 0;
};

class DefaultHttpServlet : public HttpServlet {
 public:
  DefaultHttpServlet() = default;
  ~DefaultHttpServlet() override = default;

  void Handle(HttpRequest& request, HttpResponse& response) override {
    response.SetStatus(HttpStatus::NOT_FOUND);
  }
};

class FunctionHttpServlet : public HttpServlet {
 public:
  FunctionHttpServlet(std::function<void(HttpRequest&, HttpResponse&)> func)
      : func_(std::move(func)) {}
  ~FunctionHttpServlet() override = default;
  void Handle(HttpRequest& request, HttpResponse& response) override {
    if (func_) func_(request, response);
  }

 private:
  std::function<void(HttpRequest&, HttpResponse&)> func_;
};

}  // namespace Cold::Net::Http

#endif /* NET_HTTP_HTTPSERVLET */
