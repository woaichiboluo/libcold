#include "cold/net/http/HttpRequest.h"

#include "cold/net/http/HttpCommon.h"
#include "cold/net/http/ServletContext.h"

using namespace Cold;

Net::Http::HttpRequest::HttpRequest(RawHttpRequest request,
                                    HttpResponse* response,
                                    ServletContext* context)
    : rawRequest_(std::move(request)),
      headers_(rawRequest_.GetAllHeader()),
      response_(response),
      context_(context) {
  assert(response_);
  assert(context_);
  DecodeUrlAndBody();
}

void Net::Http::HttpRequest::DecodeUrlAndBody() {
  url_ = UrlDecode(rawRequest_.GetUrl());
  std::string_view view(url_);
  auto pos = view.find_last_of('#');
  if (pos != std::string_view::npos) {
    fragment_ = view.substr(pos + 1);
    view = view.substr(0, pos);
  }
  pos = view.find('?');
  if (pos != std::string_view::npos) {
    ParseKV(view.substr(pos + 1), parameters_, '=', '&');
    url_ = url_.substr(0, pos);
  }
  // parse cookie
  if (headers_.contains("Cookie")) {
    std::string_view cookiesView = headers_["Cookie"];
    ParseKV(cookiesView, cookies_, '=', ';');
  }
  // defualt Content-Type:application/x-www-form-urlencoded
  body_ = UrlDecode(rawRequest_.GetBody());
  ParseKV(body_, parameters_, '=', '&');
}

// void Net::Http::HttpRequest::EncodeUrlAndBody() {
//   if (!parameters_.empty()) {
//     url_.push_back('?');
//     for (auto begin = parameters_.begin(); begin != parameters_.end();
//          ++begin) {
//       url_.append(begin->first);
//       url_.push_back('=');
//       url_.append(begin->second);
//       if (begin->first != parameters_.rbegin()->first) url_.push_back('&');
//     }
//   }
//   if (!fragment_.empty()) {
//     url_.push_back('#');
//     url_.append(fragment_);
//   }
//   url_ = UrlEncode(url_);
//   // defualt Content-Type:application/x-www-form-urlencoded
//   body_ = UrlEncode(body_);
// }

void Net::Http::HttpRequest::ParseKV(std::string_view kvStr,
                                     std::map<std::string, std::string>& m,
                                     char kDelim, char kvDelim) {
  while (true) {
    auto keypos = kvStr.find(kDelim);
    if (keypos == kvStr.npos) return;
    auto valuepos = kvStr.find(kvDelim, keypos + 1);
    auto kv = kvStr.substr(0, valuepos);
    auto key = kv.substr(0, keypos);
    auto value = kv.substr(keypos + 1);
    m[std::string(key)] = value;
    if (valuepos == kvStr.npos) return;
    kvStr = kvStr.substr(valuepos + 1);
  }
}

std::shared_ptr<Net::Http::HttpSession> Net::Http::HttpRequest::GetSession()
    const {
  if (cookies_.contains("SessionId")) {
    auto it = cookies_.find("SessionId");
    assert(it != cookies_.end());
    auto session = context_->GetSession(it->second);
    if (session) return session;
  }
  return CreateNewSession();
}

std::shared_ptr<Net::Http::HttpSession>
Net::Http::HttpRequest::CreateNewSession() const {
  auto session = context_->CreateSession();
  assert(response_);
  HttpCookie cookie;
  cookie.key = "SessionId";
  cookie.value = session->GetSessionId();
  cookie.path = "/";
  cookie.httpOnly = true;
  response_->AddCookie(std::move(cookie));
  return session;
}