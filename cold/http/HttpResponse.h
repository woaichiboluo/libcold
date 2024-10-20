#ifndef COLD_HTTP_HTTPRESPONSE
#define COLD_HTTP_HTTPRESPONSE

#include "HttpBody.h"
#include "HttpCommon.h"

namespace Cold::Http {

struct HttpCookie {
  std::string key;
  std::string value;
  int maxAge = -1;
  std::string domain = "";
  std::string path = "";
  bool httpOnly = true;
  bool secure = true;

  std::string Dump() const {
    std::string cookie = fmt::format("{}={}", key, value);
    if (maxAge >= 0) cookie.append(fmt::format(";Max-Age={}", maxAge));
    if (!domain.empty()) cookie.append(fmt::format(";Domain={}", domain));
    if (!path.empty()) cookie.append(fmt::format(";Path={}", path));
    if (httpOnly) cookie.append(";HttpOnly");
    if (secure) cookie.append(";Secure");
    return cookie;
  }
};

class HttpResponse {
 public:
  HttpResponse() = default;
  ~HttpResponse() = default;

  void SetHttpStatusCode(HttpStatusCode code) { code_ = code; }
  void SetHttpStatusCode(unsigned int code) {
    code_ = IntToHttpStatusCode(code);
  }
  HttpStatusCode GetHttpStatusCode() const { return code_; }

  void SetHttpVersion(HttpVersion version) { version_ = version; }

  STRING_MAP_CRUD(Header, headers_)

  void SetHttpConnectionStatus(HttpConnectionStatus status) {
    connectionStatus_ = status;
  }
  HttpConnectionStatus GetHttpConnectionStatus() const {
    return connectionStatus_;
  }

  void SetBody(std::unique_ptr<HttpBody> body) { body_ = std::move(body); }

  void AddCookie(const HttpCookie& cookie) { cookies_.push_back(cookie); }

  void MakeHeaders(std::string& headersBuf) {
    if (body_)
      body_->SetRelatedHeaders(headers_);
    else
      headers_["Content-Length"] = "0";
    headers_["Connection"] = HttpConnectionStatusToStr(connectionStatus_);
    headersBuf.append(fmt::format("{} {} {}{}", HttpVersionToStr(version_),
                                  HttpStatusCodeToInt(code_),
                                  GetHttpStatusCodeMessage(code_), kCRLF));
    for (const auto& [key, value] : headers_) {
      headersBuf.append(key);
      headersBuf.append(": ");
      headersBuf.append(value);
      headersBuf.append(kCRLF);
    }
    for (const auto& cookie : cookies_) {
      headersBuf.append("Set-Cookie: ");
      headersBuf.append(cookie.Dump());
      headersBuf.append(kCRLF);
    }
    headersBuf.append(kCRLF);
  }

  void SendRedirect(std::string_view url) {
    code_ = k302;
    headers_["Location"] = std::string(url);
  }

  // for debug
  std::string Dump() {
    std::string buf;
    MakeHeaders(buf);
    if (body_) buf.append(body_->ToRawBody());
    return buf;
  }

  Task<bool> SendBody(TcpSocket& socket) {
    if (body_) {
      co_await body_->SendBody(socket);
    }
    co_return true;
  }

 private:
  HttpStatusCode code_ = k200;
  HttpVersion version_ = kHTTP_V_11;
  HttpConnectionStatus connectionStatus_;

  std::map<std::string, std::string> headers_;
  std::unique_ptr<HttpBody> body_;
  std::vector<HttpCookie> cookies_;
};

}  // namespace Cold::Http

#endif /* COLD_HTTP_HTTPRESPONSE */
