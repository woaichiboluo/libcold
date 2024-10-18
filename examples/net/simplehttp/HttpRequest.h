#ifndef EXAMPLES_NET_SIMPLEHTTP_HTTPREQUEST
#define EXAMPLES_NET_SIMPLEHTTP_HTTPREQUEST

#include <map>
#include <string>

class HttpRequest {
 public:
  friend class RequestParser;
  HttpRequest() = default;
  ~HttpRequest() = default;

  const std::string& GetMethod() const { return method_; }
  const std::string& GetUri() const { return uri_; }
  const std::string& GetQuery() const { return query_; }
  const std::string& GetFragment() const { return fragment_; }
  const std::string& GetVersion() const { return version_; }
  const std::map<std::string, std::string>& GetHeaders() const {
    return headers_;
  }
  const std::string& GetBody() const { return body_; }

  void Reset() {
    method_.clear();
    uri_.clear();
    query_.clear();
    fragment_.clear();
    version_.clear();
    headers_.clear();
    body_.clear();
  }

 private:
  std::string method_;
  std::string uri_;
  std::string query_;
  std::string fragment_;
  std::string version_;
  std::map<std::string, std::string> headers_;
  std::string body_;
};

#endif /* EXAMPLES_NET_SIMPLEHTTP_HTTPREQUEST */
