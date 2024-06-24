#ifndef NET_HTTP_RAWHTTPRESPONSE
#define NET_HTTP_RAWHTTPRESPONSE

#include <map>
#include <string>

#include "cold/net/http/HttpCommon.h"
#include "third_party/fmt/include/fmt/core.h"

namespace Cold::Net::Http {

class RawHttpResponse {
 public:
  RawHttpResponse() = default;
  ~RawHttpResponse() = default;

  void SetMethod(std::string method) { method_ = std::move(method); }
  void SetStatus(std::string method) { method_ = std::move(method); }
  void SetVersion(std::string version) { version_ = std::move(version); }
  void SetBody(std::string body) { body_ = std::move(body); }
  HttpStatus GetStatus() const { return status_; }
  std::string_view GetMethod() const { return method_; }
  std::string_view GetVersion() const { return version_; }
  std::string_view GetBody() const { return body_; }
  // for headers
  MAP_CRUD(Header, headers_)
  void AddCookie(std::string cookie) { cookies_.push_back(std::move(cookie)); }
  // for debug
  std::string Dump() const {
    auto ret = fmt::format("{} {} {}{}", version_, static_cast<int>(status_),
                           HttpStatusToHttpStatusMsg(status_), kCRLF);
    for (const auto& [key, value] : headers_) {
      ret.append(key);
      ret.push_back(':');
      ret.push_back(' ');
      ret.append(value);
      ret.append(kCRLF);
    }
    for (const auto& cookie : cookies_) {
      ret.append("Set-Cookie: ");
      ret.append(cookie);
      ret.append(kCRLF);
    }
    ret.append(kCRLF);
    ret.append(body_);
    return ret;
  }

 private:
  HttpStatus status_ = HttpStatus::OK;
  std::string method_;
  std::string version_;
  std::map<std::string, std::string> headers_;
  std::vector<std::string> cookies_;
  std::string body_;
};
}  // namespace Cold::Net::Http

#endif /* NET_HTTP_RAWHTTPRESPONSE */
