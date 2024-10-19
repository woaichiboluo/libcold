#ifndef COLD_HTTP_HTTPSERVLET
#define COLD_HTTP_HTTPSERVLET

namespace Cold::Http {

class HttpServlet {
 public:
  HttpServlet() = default;
  virtual ~HttpServlet() = default;
};

}  // namespace Cold::Http

#endif /* COLD_HTTP_HTTPSERVLET */
