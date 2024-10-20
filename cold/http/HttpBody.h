#ifndef COLD_HTTP_HTTPBODY
#define COLD_HTTP_HTTPBODY

#include <map>
#include <string>

#include "../net/TcpSocket.h"

namespace Cold::Http {

class HttpBody {
 public:
  HttpBody() = default;
  virtual ~HttpBody() = default;

  HttpBody(const HttpBody&) = delete;
  HttpBody& operator=(const HttpBody&) = delete;

  virtual void SetRelatedHeaders(
      std::map<std::string, std::string>& headers) = 0;
  virtual Task<bool> SendBody(TcpSocket& socket) = 0;

  virtual std::string ToRawBody() const { return ""; }
};

class HtmlTextBody : public HttpBody {
 public:
  HtmlTextBody() = default;
  ~HtmlTextBody() override = default;

  void SetRelatedHeaders(std::map<std::string, std::string>& headers) override {
    headers["Content-Length"] = std::to_string(text_.size());
    headers["Content-Type"] = "text/html;charset=utf-8";
  }

  Task<bool> SendBody(TcpSocket& socket) override {
    auto n = co_await socket.WriteN(text_.data(), text_.size());
    co_return n == static_cast<ssize_t>(text_.size());
  }

  std::string ToRawBody() const override { return text_; }

  const std::string& GetText() const { return text_; }
  std::string& GetMutableText() { return text_; }

  void Append(std::string_view c) { text_.append(c); }

  void Append(const char* data, size_t length) { text_.append(data, length); }

  void Append(const char* data) { text_.append(std::string_view{data}); }

  HtmlTextBody& operator<<(std::string_view c) {
    text_.append(c);
    return *this;
  }

  HtmlTextBody& operator<<(const char* data) {
    text_.append(data);
    return *this;
  }

 private:
  std::string text_;
};

template <typename T, typename... Args>
std::unique_ptr<T> MakeHttpBody(Args&&... args) {
  return std::make_unique<T>(std::forward<Args>(args)...);
}

}  // namespace Cold::Http

#endif /* COLD_HTTP_HTTPBODY */
