#ifndef NET_HTTP_STATICFILESERVLET
#define NET_HTTP_STATICFILESERVLET

#include "cold/net/http/HttpServlet.h"
#include "cold/net/http/StaticFileBody.h"

namespace Cold::Net::Http {

class StaticFileServlet : public HttpServlet {
 public:
  explicit StaticFileServlet(std::string docRoot)
      : docRoot_(std::move(docRoot)) {}

  ~StaticFileServlet() = default;

  void Handle(HttpRequest& request, HttpResponse& response) override {
    auto body = std::make_unique<StaticFileBody>(std::string(request.GetUrl()),
                                                 docRoot_, &response);
    response.SetBody(std::move(body));
  }

 private:
  std::string docRoot_;
};

}  // namespace Cold::Net::Http

#endif /* NET_HTTP_STATICFILESERVLET */
