#ifndef NET_HTTP_SERVLET
#define NET_HTTP_SERVLET

#include <string_view>

namespace Cold::Net::Http {

class HttpRequest;
class HttpResponse;

class DispathcherSevlet {
 public:
  DispathcherSevlet() = default;
  virtual ~DispathcherSevlet() = default;
  virtual void DoDispathcer(std::string_view url) = 0;
};

class HttpServlet {
 public:
  HttpServlet() = default;
  virtual ~HttpServlet() = default;

  void DoHttp(HttpRequest& request, HttpResponse& response);
};

}  // namespace Cold::Net::Http

#endif /* NET_HTTP_SERVLET */
