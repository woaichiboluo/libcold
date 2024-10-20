#ifndef COLD_HTTP_HTTPREQUEST
#define COLD_HTTP_HTTPREQUEST

#include <any>
#include <memory>

#include "../util/StringUtil.h"
#include "../util/Url.h"
#include "RawHttpRequest.h"
#include "ServletContext.h"

namespace Cold::Http {

// for server
class HttpRequest {
 public:
  friend class HttpServer;

  HttpRequest(RawHttpRequest request)
      : rawRequest_(std::move(request)), headers_(rawRequest_.GetAllHeader()) {
    version_ = StrToHttpVersion(rawRequest_.GetVersion());
    method_ = StrToHttpMethod(rawRequest_.GetMethod());
    // parse url
    auto tuple = DecodeHttpRequestUrl(rawRequest_.GetUrl());
    url_ = std::move(std::get<0>(tuple));
    query_ = std::move(std::get<1>(tuple));
    ParseKV(query_, '=', '&', parameters_);
    fragment_ = std::move(std::get<2>(tuple));
    // parse cookie
    auto it = headers_.find("Cookie");
    if (it != headers_.end()) {
      ParseKV(it->second, '=', ';', cookies_);
    }
    // parse urlencoded body
    it = headers_.find("Content-Type");
    if (it != headers_.end() &&
        it->second == "application/x-www-form-urlencoded") {
      body_ = UrlDecode(rawRequest_.GetBody());
      ParseKV(body_, '=', '&', parameters_);
    }
    // parse connection status
    it = headers_.find("Connection");
    if (version_ == kHTTP_V_10 || it == headers_.end()) {
      connectionStatus_ = kClose;
    } else {
      if (it->second == "keep-alive") {
        connectionStatus_ = kKeepAlive;
      } else if (it->second == "Upgrade") {
        connectionStatus_ = kUpgrade;
      } else {
        connectionStatus_ = kClose;
      }
    }
  }

  ~HttpRequest() = default;

  HttpRequest(const HttpRequest&) = delete;
  HttpRequest& operator=(const HttpRequest&) = delete;

  STRING_MAP_CRUD(Cookie, cookies_)
  STRING_MAP_CRUD(Header, headers_);
  STRING_MAP_CRUD(Parameter, parameters_)
  ANY_MAP_CRUD(Attribute, attributes_)

  HttpVersion GetVersion() const { return version_; }
  const std::string& GetRawVersion() const { return rawRequest_.GetVersion(); }

  HttpMethod GetMethod() const { return method_; }
  const std::string& GetRawMethod() const { return rawRequest_.GetMethod(); }

  const std::string& GetUrl() const { return url_; }
  const std::string& GetRawUrl() const { return rawRequest_.GetUrl(); }
  const std::string& GetQuery() const { return query_; }
  const std::string& GetFragment() const { return fragment_; }
  const std::string& GetBody() const { return fragment_; }
  const std::string& GetRawBody() const { return rawRequest_.GetBody(); }

  ServletContext* GetServletContext() const { return context_; }

  HttpConnectionStatus GetConnectionStatus() const { return connectionStatus_; }

  const RawHttpRequest& GetRawRequest() const { return rawRequest_; }

  std::shared_ptr<HttpSession> GetSession() {
    auto it = cookies_.find("SessionID");
    std::shared_ptr<HttpSession> session;
    if (it != cookies_.end()) {
      session = context_->GetSession(it->second);
    }
    if (!session) {
      session = context_->CreateSession(*response_);
    }
    return session;
  }

 private:
  void SetSevlertContext(ServletContext* context) { context_ = context; }
  void SetHttpResponse(HttpResponse* response) { response_ = response; }

  RawHttpRequest rawRequest_;

  HttpVersion version_;
  HttpMethod method_;
  HttpConnectionStatus connectionStatus_;

  std::string url_;
  std::string query_;
  std::string fragment_;
  std::string body_;

  std::map<std::string, std::string>& headers_;
  std::map<std::string, std::string> parameters_;
  std::map<std::string, std::string> cookies_;
  std::map<std::string, std::any> attributes_;

  ServletContext* context_;
  // when create session need to use this
  HttpResponse* response_;
};

}  // namespace Cold::Http

#endif /* COLD_HTTP_HTTPREQUEST */
