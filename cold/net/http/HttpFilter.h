#ifndef NET_HTTP_HTTPFILTER
#define NET_HTTP_HTTPFILTER

namespace Cold::Net::Http {

class HttpRequest;
class HttpResponse;

class HttpFilter {
 public:
  HttpFilter() = default;
  virtual ~HttpFilter() = default;

  // true 放行 false 不放行
  virtual bool DoFilter(HttpRequest& request, HttpResponse& response) = 0;
};

}  // namespace Cold::Net::Http

#endif /* NET_HTTP_HTTPFILTER */
