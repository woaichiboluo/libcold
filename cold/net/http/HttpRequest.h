#ifndef NET_HTTP_HTTPREQUEST
#define NET_HTTP_HTTPREQUEST

#include <algorithm>
#include <cctype>
#include <map>
#include <string>
#include <string_view>

#include "cold/log/Logger.h"

namespace Cold::Net {

class HttpRequest {
 public:
  HttpRequest() = default;
  ~HttpRequest() = default;

  void SetMethod(std::string method) {
    std::transform(method.begin(), method.end(), method.begin(), toupper);
    method_ = std::move(method);
  }

  void SetUrl(std::string url) { url_ = url; }
  void SetVersion(std::string version) { version_ = std::move(version); }
  void SetHeader(std::string key, std::string value) {
    headers_.emplace(std::move(key), std::move(value));
  }
  void SetBody(std::string body) { body_ = std::move(body); }
  std::string ToRawRequest() const;

  std::string_view GetMethod() const { return method_; }
  std::string_view GetUrl() const { return url_; }
  std::string_view GetVersion() const { return version_; }

  const std::map<std::string, std::string>& GetHeaders() const {
    return headers_;
  }

  bool HasHeader(const std::string& key) const {
    return headers_.find(key) != headers_.end();
  }

  std::string_view GetHeader(const std::string& key) const {
    auto it = headers_.find(key);
    if (it == headers_.end()) {
      Base::INFO("not found key");
      return "";
    }
    return it->second;
  }

  std::string_view GetBody() const { return body_; }

  void DecodeUrlAndBody();

  void EncodeUrlAndBody();

  void SetAttribute(std::string key, std::string value);
  void RemoveAttribute(std::string key);
  std::string_view GetAttribute(std::string key);

 private:
  std::string method_;
  std::string url_;
  std::string version_;
  std::map<std::string, std::string> headers_;
  std::string body_;

  std::string query_;
  std::string fragment_;
  std::map<std::string, std::string> attributes_;
  std::map<std::string, std::string> parameters_;
};

}  // namespace Cold::Net

#endif /* NET_HTTP_HTTPREQUEST */
