#ifndef COLD_HTTP_RAWHTTPREQUEST
#define COLD_HTTP_RAWHTTPREQUEST

#include <map>
#include <string>

#include "../detail/fmt.h"
#include "HttpCommon.h"

namespace Cold::Http {

class RawHttpRequest {
 public:
  RawHttpRequest() = default;
  ~RawHttpRequest() = default;

  std::string Dump() const {
    std::string ret;
    ret.append(fmt::format("{} {} {}{}", method_, url_, version_, kCRLF));
    for (const auto& [key, value] : headers_) {
      ret.append(fmt::format("{}: {}{}", key, value, kCRLF));
    }
    ret.append(kCRLF);
    ret.append(body_);
    return ret;
  }

  STRING_MAP_CRUD(Header, headers_)

  void SetMethod(std::string method) { method_ = std::move(method); }
  void SetUrl(std::string url) { url_ = url; }
  void SetVersion(std::string version) { version_ = std::move(version); }
  void SetBody(std::string body) { body_ = std::move(body); }

  const std::string& GetMethod() const { return method_; }
  const std::string& GetUrl() const { return url_; }
  const std::string& GetVersion() const { return version_; }
  const std::string& GetBody() const { return body_; }

 private:
  // undecode format maybe /addr?param1=1&param2=2
  std::string method_;
  std::string url_;
  std::string version_;
  std::map<std::string, std::string> headers_;
  std::string body_;
};

}  // namespace Cold::Http

#endif /* COLD_HTTP_RAWHTTPREQUEST */
