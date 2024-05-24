#ifndef NET_HTTP_HTTPREQUESTPARSER
#define NET_HTTP_HTTPREQUESTPARSER

#include <cerrno>
#include <queue>

#include "cold/net/http/HttpRequest.h"
#include "cold/util/Config.h"
#include "third_party/llhttp/llhttp.h"

namespace Cold::Net {

class HttpRequestParser {
 public:
  HttpRequestParser();
  ~HttpRequestParser() = default;

  HttpRequestParser(const HttpRequestParser&) = delete;
  HttpRequestParser& operator=(const HttpRequestParser&) = delete;

  bool Parse(const char* data, size_t len);

  bool HasRequest() const { return !requestQueue_.empty(); }
  size_t RequestQueueSize() const { return requestQueue_.size(); }
  HttpRequest TakeRequest();
  const char* GetBadReason() const {
    if (parser_.reason) return parser_.reason;
    return "No error";
  }

 private:
  static int OnMethod(llhttp_t*);

  static int OnUrl(llhttp_t*, const char* at, size_t length);
  static int OnUrlComplete(llhttp_t*);

  static int OnVersion(llhttp_t*, const char* at, size_t length);
  static int OnVersionComplete(llhttp_t*);

  static int OnHeaderField(llhttp_t*, const char* at, size_t length);
  static int OnHeaderFieldComplete(llhttp_t*);

  static int OnHeaderValue(llhttp_t*, const char* at, size_t length);
  static int OnHeaderValueComplete(llhttp_t*);

  static int OnBody(llhttp_t*, const char* at, size_t length);
  static int OnMessageComplete(llhttp_t*);

  static size_t GetMaxUrlSize() {
    static size_t value = Base::Config::GetGloablDefaultConfig().GetOrDefault(
        "/http/max-url-size", 190000ull);  // 185K
    return value;
  }

  static size_t GetMaxHeaderFieldSize() {
    static size_t value = Base::Config::GetGloablDefaultConfig().GetOrDefault(
        "/http/max-header-field-size", 1024ull);  // 1K
    return value;
  }

  static size_t GetMaxHeaderValueSize() {
    static size_t value = Base::Config::GetGloablDefaultConfig().GetOrDefault(
        "/http/max-header-value-size", 10 * 1024ull);  // 10K
    return value;
  }

  static size_t GetMaxHeaderCount() {
    static size_t value = Base::Config::GetGloablDefaultConfig().GetOrDefault(
        "/http/max-headers-count", 100ull);  // 30M
    return value;
  }

  static size_t GetMaxBodySize() {
    static size_t value = Base::Config::GetGloablDefaultConfig().GetOrDefault(
        "/http/max-body-size", 1024 * 1024ull);  // 1M
    return value;
  }

  llhttp_t parser_;
  llhttp_settings_t settings_;
  HttpRequest curRequest_;
  bool checkContentLength_ = true;
  std::string buf_;
  std::string bufForHeaderValue_;
  std::queue<HttpRequest> requestQueue_;
};

}  // namespace Cold::Net

#endif /* NET_HTTP_HTTPREQUESTPARSER */
