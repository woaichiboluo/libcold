#ifndef NET_HTTP_HTTPREQUEST
#define NET_HTTP_HTTPREQUEST

#include <algorithm>
#include <cctype>
#include <map>
#include <string>
#include <string_view>

#include "cold/net/http/RawHttpRequest.h"

namespace Cold::Net::Http {

class ServletContext;
class HttpResponse;

class HttpRequest {
 public:
  HttpRequest(RawHttpRequest request, HttpResponse* response,
              ServletContext* context);

  ~HttpRequest() = default;

  HttpRequest(const HttpRequest&) = delete;
  HttpRequest& operator=(const HttpRequest&) = delete;

  std::string_view GetMethod() const { return rawRequest_.GetMethod(); }
  std::string_view GetUrl() const { return url_; }
  std::string_view GetVersion() const { return rawRequest_.GetVersion(); }

  // for parameters
  MAP_READ(Parameter, parameters_)
  // for attributes
  MAP_CRUD(Attribute, attributes_);
  // for cookies
  MAP_READ(Cookie, cookies_)

  bool IsKeepAlive() const {
    if (rawRequest_.HasHeader("Connection"))
      return rawRequest_.GetHeader("Connection") == "keep-alive";
    return rawRequest_.GetVersion() == "HTTP/1.1";
  }

  ServletContext* GetServletContext() const { return context_; }
  HttpResponse* GetHttpResponse() const { return response_; }
  const RawHttpRequest& GetRawRequest() const { return rawRequest_; }

 private:
  void DecodeUrlAndBody();
  void ParseKV(std::string_view kvStr, std::map<std::string, std::string>& m,
               char kDelim, char kvDelim);

  RawHttpRequest rawRequest_;
  std::map<std::string, std::string> headers_;

  std::string url_;
  std::string query_;
  std::string fragment_;
  std::string body_;

  std::map<std::string, std::string> attributes_;
  std::map<std::string, std::string> parameters_;
  std::map<std::string, std::string> cookies_;

  HttpResponse* response_;
  ServletContext* context_;
};

}  // namespace Cold::Net::Http

#endif /* NET_HTTP_HTTPREQUEST */
