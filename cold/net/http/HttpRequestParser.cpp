#include "cold/net/http/HttpRequestParser.h"

#include <cassert>
#include <charconv>

#include "third_party/llhttp/llhttp.h"

using namespace Cold;

Net::HttpRequestParser::HttpRequestParser() {
  llhttp_settings_init(&settings_);
  llhttp_init(&parser_, HTTP_REQUEST, &settings_);
  parser_.data = this;
  settings_.on_message_complete = nullptr;
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

int Net::HttpRequestParser::OnMethod(llhttp_t* parser) {
  auto self = static_cast<HttpRequestParser*>(parser->data);
  self->curRequest_.SetMethod(
      llhttp_method_name(static_cast<llhttp_method>(parser->method)));
  return 0;
}

int Net::HttpRequestParser::OnUrl(llhttp_t* parser, const char* at,
                                  size_t length) {
  auto self = static_cast<HttpRequestParser*>(parser->data);
  if (self->buf_.size() + length > GetMaxUrlSize()) {
    llhttp_set_error_reason(parser, "Url size is too large");
    return HPE_USER;
  }
  self->buf_.append(at, length);
  return 0;
}

int Net::HttpRequestParser::OnUrlComplete(llhttp_t* parser) {
  auto self = static_cast<HttpRequestParser*>(parser->data);
  self->curRequest_.SetUrl(std::move(self->buf_));
  return 0;
}

int Net::HttpRequestParser::OnVersion(llhttp_t* parser, const char* at,
                                      size_t length) {
  auto self = static_cast<HttpRequestParser*>(parser->data);
  self->buf_.append(at, length);
  return 0;
}

int Net::HttpRequestParser::OnVersionComplete(llhttp_t* parser) {
  auto self = static_cast<HttpRequestParser*>(parser->data);
  std::string v = "HTTP/";
  v += self->buf_;
  self->curRequest_.SetVersion(std::move(v));
  self->buf_.clear();
  return 0;
}

int Net::HttpRequestParser::OnHeaderField(llhttp_t* parser, const char* at,
                                          size_t length) {
  auto self = static_cast<HttpRequestParser*>(parser->data);
  if (self->curRequest_.GetHeaders().size() + 1 > GetMaxHeaderCount()) {
    llhttp_set_error_reason(parser, "Headers is too large");
    return HPE_USER;
  }
  if (self->buf_.size() + length > GetMaxHeaderFieldSize()) {
    llhttp_set_error_reason(parser, "Header field size is too large");
    return HPE_USER;
  }
  self->buf_.append(at, length);
  return 0;
}

int Net::HttpRequestParser::OnHeaderFieldComplete(llhttp_t* parser) {
  return 0;
}

int Net::HttpRequestParser::OnHeaderValue(llhttp_t* parser, const char* at,
                                          size_t length) {
  auto self = static_cast<HttpRequestParser*>(parser->data);
  if (self->bufForHeaderValue_.size() + length > GetMaxHeaderValueSize()) {
    llhttp_set_error_reason(parser, "Header value size is too large");
    return HPE_USER;
  }
  self->bufForHeaderValue_.append(at, length);
  return 0;
}

int Net::HttpRequestParser::OnHeaderValueComplete(llhttp_t* parser) {
  auto self = static_cast<HttpRequestParser*>(parser->data);
  self->curRequest_.SetHeader(std::move(self->buf_),
                              std::move(self->bufForHeaderValue_));
  return 0;
}

int Net::HttpRequestParser::OnBody(llhttp_t* parser, const char* at,
                                   size_t length) {
  auto self = static_cast<HttpRequestParser*>(parser->data);
  if (self->checkContentLength_) {
    size_t contentLength = 0;
    auto str = self->curRequest_.GetHeader("Content-Length");
    auto [_, ec] = std::from_chars(str.begin(), str.end(), contentLength);
    if (ec != std::errc()) {
      llhttp_set_error_reason(parser, "Bad Content-Length");
      return HPE_USER;
    }
    if (contentLength > GetMaxBodySize()) {
      llhttp_set_error_reason(parser, "Body size is too large");
      return HPE_USER;
    }
    self->checkContentLength_ = false;
  }
  self->buf_.append(at, length);
  return 0;
}

int Net::HttpRequestParser::OnMessageComplete(llhttp_t* parser) {
  auto self = static_cast<HttpRequestParser*>(parser->data);
  self->curRequest_.SetBody(std::move(self->buf_));
  self->requestQueue_.push(std::move(self->curRequest_));
  self->curRequest_ = HttpRequest();
  self->checkContentLength_ = true;
  return 0;
}

bool Net::HttpRequestParser::Parse(const char* data, size_t len) {
  auto errcode = llhttp_execute(&parser_, data, len);
  return errcode == HPE_OK;
}

Net::HttpRequest Net::HttpRequestParser::TakeRequest() {
  assert(!requestQueue_.empty());
  auto req = std::move(requestQueue_.front());
  requestQueue_.pop();
  return req;
}