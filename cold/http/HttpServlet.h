#ifndef COLD_HTTP_HTTPSERVLET
#define COLD_HTTP_HTTPSERVLET

#include <functional>

namespace Cold::Http {

class HttpRequest;
class HttpResponse;

class HttpServlet {
 public:
  HttpServlet() = default;
  virtual ~HttpServlet() = default;

  HttpServlet(const HttpServlet&) = delete;
  HttpServlet& operator=(const HttpServlet&) = delete;

  virtual void DoService(HttpRequest& request, HttpResponse& response) {}
};

class FunctionServlet : public HttpServlet {
 public:
  FunctionServlet(std::function<void(HttpRequest&, HttpResponse&)> func)
      : func_(std::move(func)) {}
  ~FunctionServlet() override = default;

 private:
  void DoService(HttpRequest& request, HttpResponse& response) override {
    func_(request, response);
  }

  std::function<void(HttpRequest&, HttpResponse&)> func_;
};

}  // namespace Cold::Http

#endif /* COLD_HTTP_HTTPSERVLET */
