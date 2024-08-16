#ifndef NET_HTTP_HTTPREQUEST
#define NET_HTTP_HTTPREQUEST

#include <any>
#include <cctype>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>

#include "cold/net/http/RawHttpRequest.h"

namespace Cold::Net::Http {

class ServletContext;
class HttpResponse;
class HttpSession;

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
  std::shared_ptr<HttpSession> GetSession() const;

  // for parameters
  MAP_READ(Parameter, parameters_)
  // for cookies
  MAP_READ(Cookie, cookies_)
  // for headers
  MAP_READ(Header, headers_)

  // for attributes
  template <typename T>
  void SetAttribute(std::string key, T value) requires
      std::is_copy_assignable_v<T> && std::is_copy_constructible_v<T> {
    attributes_[std::move(key)] = std::any(std::move(value));
  }

  void SetAttribute(std::string key, const char* value) {
    attributes_[std::move(key)] = std::any(std::string(value));
  }

  template <typename T>
  T* GetAttribute(const std::string& key) {
    auto it = attributes_.find(key);
    if (it != attributes_.end()) {
      try {
        return std::any_cast<T>(&it->second);
      } catch (...) {
        return nullptr;
      }
    }
    return nullptr;
  }

  bool HasAttribute(const std::string& key) const {
    return attributes_.contains(key);
  }

  void RemoveAttribute(const std::string& key) { attributes_.erase(key); }

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
  std::shared_ptr<HttpSession> CreateNewSession() const;

  RawHttpRequest rawRequest_;
  std::map<std::string, std::string>& headers_;

  std::string url_;
  std::string query_;
  std::string fragment_;
  std::string body_;

  std::map<std::string, std::any> attributes_;
  std::map<std::string, std::string> parameters_;
  std::map<std::string, std::string> cookies_;

  HttpResponse* response_;
  ServletContext* context_;
};

}  // namespace Cold::Net::Http

#endif /* NET_HTTP_HTTPREQUEST */
