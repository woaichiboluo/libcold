#ifndef SIMPLE_SIMPLEHTTP_HTTPREQUEST
#define SIMPLE_SIMPLEHTTP_HTTPREQUEST

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

 private:
  std::string method_;
  std::string uri_;
  std::string query_;
  std::string fragment_;
  std::string version_;
  std::map<std::string, std::string> headers_;
  std::string body_;
};

#endif /* SIMPLE_SIMPLEHTTP_HTTPREQUEST */
