#ifndef COLD_HTTP_REQUESTPARSER
#define COLD_HTTP_REQUESTPARSER

#include <llhttp.h>

#include <cassert>
#include <charconv>
#include <memory>
#include <queue>
#include <string_view>

#include "RawHttpRequest.h"

namespace Cold::Http {
class HttpRequestParser {
 public:
  HttpRequestParser() : checker_(std::make_unique<Checker>()) {
    llhttp_settings_init(&settings_);
    llhttp_init(&parser_, HTTP_REQUEST, &settings_);
    parser_.data = this;
    settings_.on_method_complete = &HttpRequestParser::OnMethod;

    settings_.on_url = &HttpRequestParser::OnUrl;
    settings_.on_url_complete = &HttpRequestParser::OnUrlComplete;

    settings_.on_version = &HttpRequestParser::OnVersion;
    settings_.on_version_complete = &HttpRequestParser::OnVersionComplete;

    settings_.on_header_field = &HttpRequestParser::OnHeaderField;
    settings_.on_header_field_complete =
        &HttpRequestParser::OnHeaderFieldComplete;

    settings_.on_header_value = &HttpRequestParser::OnHeaderValue;
    settings_.on_header_value_complete =
        &HttpRequestParser::OnHeaderValueComplete;

    settings_.on_body = &HttpRequestParser::OnBody;
    settings_.on_message_complete = &HttpRequestParser::OnMessageComplete;
  }

  ~HttpRequestParser() = default;

  HttpRequestParser(const HttpRequestParser&) = delete;
  HttpRequestParser& operator=(const HttpRequestParser&) = delete;

  // checker should only judge the validity of the request, don't check the size
  // the size should be checked by the parser
  class Checker {
   public:
    friend class HttpRequestParser;
    Checker() = default;
    virtual ~Checker() = default;

    virtual bool OnMethod(std::string_view method) { return true; }

    virtual bool OnUrl(std::string_view url) { return true; }

    virtual bool OnVersion(std::string_view version) {
      if (version != "HTTP/1.1" && version != "HTTP/1.0") return false;
      return true;
    }

    virtual bool OnHeader(std::string_view key, std::string_view value) {
      if (key.size() > maxHeaderFieldSize_ ||
          value.size() > maxHeaderValueSize_)
        return false;
      if (key == "Content-Length") {
        size_t contentLength = 0;
        auto [_, ec] =
            std::from_chars(value.begin(), value.end(), contentLength);
        if (ec != std::errc()) return false;
        if (contentLength > maxBodySize_) return false;
      }
      return true;
    }

    virtual bool OnBody(std::string_view body) {
      if (body.size() > maxBodySize_) return false;
      return true;
    }

   protected:
    bool onMethod_ = false;
    bool onUrl_ = true;
    size_t maxUrlSize_ = 190000;
    bool onVersion_ = true;
    size_t maxHeaderFieldSize_ = 1024;
    size_t maxHeaderValueSize_ = 10 * 1024;
    bool onHeader_ = true;
    bool onBody_ = true;
    size_t maxBodySize_ = 1024 * 1024;
  };

  bool Parse(const char* data, size_t len) {
    auto errcode = llhttp_execute(&parser_, data, len);
    if (errcode == HPE_PAUSED_UPGRADE) {
      llhttp_resume_after_upgrade(&parser_);
      errcode = llhttp_execute(&parser_, nullptr, 0);
    }
    return errcode == HPE_OK;
  }

  bool HasRequest() const { return !requestQueue_.empty(); }

  size_t RequestQueueSize() const { return requestQueue_.size(); }

  RawHttpRequest TakeRequest() {
    assert(!requestQueue_.empty());
    auto ret = std::move(requestQueue_.front());
    requestQueue_.pop();
    return ret;
  }

  const char* GetBadReason() const {
    if (parser_.reason) return parser_.reason;
    return "No error";
  }

 private:
  static int OnMethod(llhttp_t* parser) {
    auto self = static_cast<HttpRequestParser*>(parser->data);
    if (self->checker_->onMethod_ &&
        !self->checker_->OnMethod(self->curRequest_.GetMethod())) {
      return HPE_USER;
    }
    self->curRequest_.SetMethod(
        llhttp_method_name(static_cast<llhttp_method>(parser->method)));
    return 0;
  }

  static int OnUrl(llhttp_t* parser, const char* at, size_t length) {
    auto self = static_cast<HttpRequestParser*>(parser->data);
    if (self->checker_->onUrl_ &&
        self->buf_.size() + length > self->checker_->maxUrlSize_) {
      llhttp_set_error_reason(parser, "Url size is too large");
      return HPE_USER;
    }
    self->buf_.append(at, length);
    return 0;
  }

  static int OnUrlComplete(llhttp_t* parser) {
    auto self = static_cast<HttpRequestParser*>(parser->data);
    if (self->checker_->onUrl_ && !self->checker_->OnUrl(self->buf_)) {
      return HPE_USER;
    }
    self->curRequest_.SetUrl(std::move(self->buf_));
    return 0;
  }

  static int OnVersion(llhttp_t* parser, const char* at, size_t length) {
    auto self = static_cast<HttpRequestParser*>(parser->data);
    self->buf_.append(at, length);
    return 0;
  }

  static int OnVersionComplete(llhttp_t* parser) {
    auto self = static_cast<HttpRequestParser*>(parser->data);
    std::string v = "HTTP/";
    v += self->buf_;
    if (self->checker_->onVersion_ && !self->checker_->OnVersion(v)) {
      return HPE_USER;
    }
    self->curRequest_.SetVersion(std::move(v));
    self->buf_.clear();
    return 0;
  }

  static int OnHeaderField(llhttp_t* parser, const char* at, size_t length) {
    auto self = static_cast<HttpRequestParser*>(parser->data);
    if (self->checker_->onHeader_ &&
        self->buf_.size() + length > self->checker_->maxHeaderFieldSize_) {
      llhttp_set_error_reason(parser, "Header field size is too large");
      return HPE_USER;
    }
    self->buf_.append(at, length);
    return 0;
  }

  static int OnHeaderFieldComplete(llhttp_t* parser) {
    auto self = static_cast<HttpRequestParser*>(parser->data);
    self->headerField_ = std::move(self->buf_);
    return 0;
  }

  static int OnHeaderValue(llhttp_t* parser, const char* at, size_t length) {
    auto self = static_cast<HttpRequestParser*>(parser->data);
    if (self->checker_->onHeader_ &&
        self->buf_.size() + length > self->checker_->maxHeaderValueSize_) {
      llhttp_set_error_reason(parser, "Header value size is too large");
      return HPE_USER;
    }
    self->buf_.append(at, length);
    return 0;
  }

  static int OnHeaderValueComplete(llhttp_t* parser) {
    auto self = static_cast<HttpRequestParser*>(parser->data);
    if (self->checker_->onHeader_ &&
        !self->checker_->OnHeader(self->headerField_, self->buf_)) {
      return HPE_USER;
    }
    self->curRequest_.SetHeader(std::move(self->headerField_),
                                std::move(self->buf_));
    return 0;
  }

  static int OnBody(llhttp_t* parser, const char* at, size_t length) {
    auto self = static_cast<HttpRequestParser*>(parser->data);
    if (self->checker_->onBody_ &&
        self->buf_.size() + length > self->checker_->maxBodySize_) {
      llhttp_set_error_reason(parser, "Bad Body");
      return HPE_USER;
    }
    self->buf_.append(at, length);
    return 0;
  }

  static int OnMessageComplete(llhttp_t* parser) {
    auto self = static_cast<HttpRequestParser*>(parser->data);
    self->curRequest_.SetBody(std::move(self->buf_));
    self->requestQueue_.push(std::move(self->curRequest_));
    self->curRequest_ = RawHttpRequest();
    return 0;
  }

  std::unique_ptr<Checker> checker_;
  llhttp_t parser_;
  llhttp_settings_t settings_;
  RawHttpRequest curRequest_;
  std::string buf_;
  std::string headerField_;
  std::queue<RawHttpRequest> requestQueue_;
};

}  // namespace Cold::Http

#endif /* COLD_HTTP_REQUESTPARSER */
