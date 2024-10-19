#ifndef COLD_HTTP_HTTPFILTER
#define COLD_HTTP_HTTPFILTER

namespace Cold::Http {
class HttpFilter {
 public:
  HttpFilter() = default;
  virtual ~HttpFilter() = default;
};
}  // namespace Cold::Http

#endif /* COLD_HTTP_HTTPFILTER */
