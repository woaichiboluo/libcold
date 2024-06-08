#ifndef NET_HTTP_HTTPRESPONSE
#define NET_HTTP_HTTPRESPONSE

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "cold/net/TcpSocket.h"
#include "cold/net/http/HttpCommon.h"

namespace Cold::Net::Http {

class HttpResponseBody {
 public:
  HttpResponseBody() = default;
  virtual ~HttpResponseBody() = default;
  HttpResponseBody(const HttpResponseBody&) = delete;
  HttpResponseBody& operator=(const HttpResponseBody&) = delete;

  virtual void SetRelatedHeaders(
      std::map<std::string, std::string>& headers) const = 0;
  // SendComplete should return true else returnf false
  virtual Base::Task<bool> Send(Net::TcpSocket& socket) = 0;
  // for debug
  virtual std::string ToRawBody() const { return ""; }
};

class TextBody : public HttpResponseBody {
 public:
  void SetRelatedHeaders(
      std::map<std::string, std::string>& headers) const override {
    headers["Content-Length"] = std::to_string(body_.size());
  }

  Base::Task<bool> Send(Net::TcpSocket& socket) override {
    auto n = co_await socket.WriteN(body_.data(), body_.size());
    co_return n == static_cast<ssize_t>(body_.size());
  }

  std::string ToRawBody() const override { return body_; }

  void SetContent(std::string body) { body_ = std::move(body); }

 private:
  std::string body_;
};

inline std::unique_ptr<TextBody> MakeTextBody() {
  return std::make_unique<TextBody>();
}

class HttpResponse {
 public:
  HttpResponse() = default;
  ~HttpResponse() = default;

  void SetStatus(HttpStatus status) { status_ = status; }

  void SetVersion(std::string version) { version_ = std::move(version); }

  void SetHeader(std::string key, std::string value) {
    headers_[std::move(key)] = std::move(value);
  }

  void SetCloseConnection(bool close) { close_ = close; }

  void SetBody(std::unique_ptr<HttpResponseBody> body) {
    body_ = std::move(body);
  }

  void MakeHeaders(std::vector<char>& headersBuf) {
    if (body_)
      body_->SetRelatedHeaders(headers_);
    else
      headers_["Content-Length"] = "0";
    auto firstLine =
        fmt::format("{} {} {}{}", version_, static_cast<int>(status_),
                    HttpStatusToHttpStatusMsg(status_), kCRLF);
    headers_["Connection"] = close_ ? "close" : "keep-alive";
    headersBuf.insert(headersBuf.end(), firstLine.begin(), firstLine.end());
    for (const auto& [key, value] : headers_) {
      headersBuf.insert(headersBuf.end(), key.begin(), key.end());
      headersBuf.push_back(':');
      headersBuf.push_back(' ');
      headersBuf.insert(headersBuf.end(), value.begin(), value.end());
      headersBuf.insert(headersBuf.end(), kCRLF.begin(), kCRLF.end());
    }
    headersBuf.insert(headersBuf.end(), kCRLF.begin(), kCRLF.end());
  }

  Base::Task<bool> SendBody(Net::TcpSocket& socket) {
    if (body_) co_return co_await body_->Send(socket);
    co_return true;
  }

  bool IsKeepAlive() const { return !close_; }

  void SendRedirect(std::string_view url) {
    status_ = HttpStatus::FOUND;
    headers_["Location"] = url;
  }

  // for debug
  std::string ToRawResponse() const {
    std::string ret =
        fmt::format("{} {} {}{}", version_, static_cast<int>(status_),
                    HttpStatusToHttpStatusMsg(status_), kCRLF);
    auto h = headers_;
    if (body_)
      body_->SetRelatedHeaders(h);
    else
      h["Content-Length"] = "0";
    h["Connection"] = close_ ? "close" : "keep-alive";
    for (const auto& [key, value] : h) {
      ret.append(fmt::format("{}: {}{}", key, value, kCRLF));
    }
    ret.append(kCRLF);
    if (body_) {
      auto b = body_->ToRawBody();
      ret.append(b.begin(), b.end());
    }
    return ret;
  }

 private:
  HttpStatus status_ = HttpStatus::OK;
  std::string version_ = "HTTP/1.1";
  std::map<std::string, std::string> headers_;
  std::unique_ptr<HttpResponseBody> body_;
  bool close_;
};

}  // namespace Cold::Net::Http

#endif /* NET_HTTP_HTTPRESPONSE */
