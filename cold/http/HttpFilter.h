#ifndef COLD_HTTP_HTTPFILTER
#define COLD_HTTP_HTTPFILTER

namespace Cold::Http {

class HttpRequest;
class HttpResponse;

class HttpFilter {
 public:
  HttpFilter() = default;
  virtual ~HttpFilter() = default;

  HttpFilter(const HttpFilter&) = delete;
  HttpFilter& operator=(const HttpFilter&) = delete;

  // true if the request is filtered
  // false if the request is not filtered
  virtual bool DoFilter(HttpRequest& request, HttpResponse& response) {
    return false;
  }
};

}  // namespace Cold::Http

#endif /* COLD_HTTP_HTTPFILTER */
