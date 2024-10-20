#ifndef COLD_HTTP_DEFAULTHTTPSERVLET
#define COLD_HTTP_DEFAULTHTTPSERVLET

#include "HttpResponse.h"
#include "HttpServlet.h"

namespace Cold::Http {

class DefaultHttpServlet : public HttpServlet {
 public:
  DefaultHttpServlet() = default;
  ~DefaultHttpServlet() override = default;

  void DoService(HttpRequest& request, HttpResponse& response) override {
    auto body = std::make_unique<HtmlTextBody>();
    body->Append(GetDefaultErrorPage(response.GetHttpStatusCode()));
    response.SetBody(std::move(body));
  }
};

}  // namespace Cold::Http

#endif /* COLD_HTTP_DEFAULTHTTPSERVLET */
