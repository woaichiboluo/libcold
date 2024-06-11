#ifndef NET_HTTP_RAWHTTPREQUEST
#define NET_HTTP_RAWHTTPREQUEST

#include <map>
#include <string>

#include "cold/net/http/HttpCommon.h"
#include "third_party/fmt/include/fmt/core.h"

namespace Cold::Net::Http {

class RawHttpRequest {
 public:
  RawHttpRequest() = default;
  ~RawHttpRequest() = default;

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
  MAP_CRUD(Header, headers_)
  // for debug
  std::string Dump() {
    std::string ret;
    ret = fmt::format("{} {} {}{}", method_, url_, version_, kCRLF);
    for (const auto& [key, value] : headers_) {
      ret.append(fmt::format("{}: {}{}", key, value, kCRLF));
    }
    ret.append(kCRLF);
    ret.append(body_);
    return ret;
  }

 private:
  std::string method_;
  std::string url_;
  std::string version_;
  std::map<std::string, std::string> headers_;
  std::string body_;
};

}  // namespace Cold::Net::Http

#endif /* NET_HTTP_RAWHTTPREQUEST */
