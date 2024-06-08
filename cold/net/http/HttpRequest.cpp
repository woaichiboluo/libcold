#include "cold/net/http/HttpRequest.h"

#include "cold/net/http/HttpCommon.h"
using namespace Cold;

void Net::Http::HttpRequest::DecodeUrlAndBody() {
  url_ = UrlDecode(url_);
  std::string_view view(url_);
  auto pos = view.find_last_of('#');
  if (pos != std::string_view::npos) {
    fragment_ = view.substr(pos + 1);
    view = view.substr(0, pos);
  }
  pos = view.find('?');
  if (pos != std::string_view::npos) {
    ParseKV(view.substr(pos + 1));
    url_ = url_.substr(0, pos);
  }
  // defualt Content-Type:application/x-www-form-urlencoded
  body_ = UrlDecode(body_);
  ParseKV(body_);
}

void Net::Http::HttpRequest::ParseKV(std::string_view kvStr) {
  while (true) {
    auto keypos = kvStr.find("=");
    if (keypos == kvStr.npos) return;
    auto valuepos = kvStr.find("&", keypos + 1);
    auto kv = kvStr.substr(0, valuepos);
    auto key = kv.substr(0, keypos);
    auto value = kv.substr(keypos + 1);
    parameters_[std::string(key)] = value;
    if (valuepos == kvStr.npos) return;
    kvStr = kvStr.substr(valuepos + 1);
  }
}