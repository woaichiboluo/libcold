#ifndef NET_HTTP_HTTPREQUEST
#define NET_HTTP_HTTPREQUEST

#include <algorithm>
#include <cctype>
#include <map>
#include <string>
#include <string_view>

namespace Cold::Net::Http {

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
  void SetBody(std::string body) { body_ = std::move(body); }

  std::string_view GetMethod() const { return method_; }
  std::string_view GetUrl() const { return url_; }
  std::string_view GetVersion() const { return version_; }
  std::string_view GetBody() const { return body_; }

  // for headers
  void SetHeader(std::string key, std::string value) {
    headers_[std::move(key)] = std::move(value);
  }

  void RemoveHeader(const std::string& key) { headers_.erase(key); }

  bool HasHeader(const std::string& key) const {
    return headers_.contains(key);
  }

  std::string_view GetHeader(const std::string& key) const {
    return FindValue(headers_, key);
  }

  const std::map<std::string, std::string>& GetHeaders() const {
    return headers_;
  }

  // for parameters
  bool HasParameter(const std::string& key) const {
    return parameters_.contains(key);
  }

  std::string_view GetParameter(const std::string& key) const {
    return FindValue(attributes_, key);
  }

  const std::map<std::string, std::string>& GetParameters() const {
    return parameters_;
  }

  // for attributes
  void SetAttribute(std::string key, std::string value) {
    attributes_[std::move(key)] = std::move(value);
  }

  void RemoveAttribute(const std::string& key) { attributes_.erase(key); }

  bool HasAttribute(const std::string& key) const {
    return attributes_.contains(key);
  }

  std::string_view GetAttribute(const std::string& key) const {
    return FindValue(attributes_, key);
  }

  const std::map<std::string, std::string>& GetAttributes() const {
    return attributes_;
  }

  // for parse
  std::string ToRawRequest() const;

  void DecodeUrlAndBody();

  void EncodeUrlAndBody();

 private:
  void ParseKV(std::string_view kvStr);

  static std::string_view FindValue(const std::map<std::string, std::string>& m,
                                    const std::string& key) {
    auto it = m.find(key);
    if (it == m.end()) return "";
    return it->second;
  }

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

}  // namespace Cold::Net::Http

#endif /* NET_HTTP_HTTPREQUEST */
