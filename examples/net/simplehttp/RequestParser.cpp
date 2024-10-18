#include "RequestParser.h"

#include "HttpRequest.h"

bool RequestParser::CheckMethod(std::string_view method, HttpRequest& request) {
  static constexpr const std::array<std::string_view, 9> kMethods = {
      "GET",     "HEAD",    "POST",  "PUT",  "DELETE",
      "CONNECT", "OPTIONS", "TRACE", "PATCH"};
  auto it = std::find(kMethods.begin(), kMethods.end(), method);
  if (it == kMethods.end()) return false;
  request.method_ = method;
  return true;
}

bool RequestParser::CheckUri(std::string_view uri, HttpRequest& request) {
  if (uri.empty()) return false;
  for (const auto& c : uri) {
    if (iscntrl(c)) return false;
  }
  auto npos = std::string_view::npos;
  request.uri_ = uri;
  auto fpos = uri.rfind("#");
  if (fpos != npos) {
    request.fragment_ = uri.substr(fpos + 1);
    uri = uri.substr(0, fpos);
  }
  auto qpos = uri.rfind("?");
  if (qpos != npos) {
    request.query_ = uri.substr(qpos + 1);
    uri = uri.substr(0, qpos);
  }
  request.uri_ = uri;
  return true;
}

bool RequestParser::CheckVersion(std::string_view version,
                                 HttpRequest& request) {
  if (version != "HTTP/1.1" && version != "HTTP/1.0") return false;
  request.version_ = version;
  return true;
}

bool RequestParser::CheckHeaderName(std::string_view headerName,
                                    HttpRequest& request) {
  if (headerName.empty()) return false;
  return true;
}

bool RequestParser::CheckHeaderValue(std::string_view headerName,
                                     std::string_view headerValue,
                                     HttpRequest& request) {
  if (headerValue.empty() || headerValue.size() == 1) return false;
  if (headerValue[0] != ' ') return false;
  headerValue = headerValue.substr(1);
  if (headerName == "Content-Length") {
    for (const auto& c : headerValue) {
      if (!isdigit(c)) return false;
    }
  }
  request.headers_[std::string(headerName)] = headerValue;
  return true;
}

bool RequestParser::CheckBody(std::string_view body, HttpRequest& request) {
  request.body_ = body;
  return true;
}

size_t RequestParser::GetBodySize(HttpRequest& request) {
  auto it = request.GetHeaders().find("Content-Length");
  if (it == request.GetHeaders().end()) return 0;
  return static_cast<size_t>(atol(it->second.c_str()));
}

RequestParser::ParseState RequestParser::Parse(std::string_view content,
                                               HttpRequest& request) {
  unsolved_ += content;
  auto npos = std::string_view::npos;
  std::string_view parseContent(unsolved_);
  while (true) {
    switch (state_) {
      case kMethod: {
        auto pos = parseContent.find(' ');
        if (pos == npos && parseContent.size() < 8) {
          unsolved_ = parseContent;
          return kKEEP;
        }
        auto method = parseContent.substr(0, pos);
        if (CheckMethod(method, request)) {
          state_ = kUri;
          parseContent = parseContent.substr(pos + 1);
          continue;
        } else {
          return kBAD;
        }
      }
      case kUri: {
        auto pos = parseContent.find(' ');
        if (pos == npos) {
          unsolved_ = parseContent;
          return kKEEP;
        }
        auto uri = parseContent.substr(0, pos);
        if (CheckUri(uri, request)) {
          state_ = kRequestVersion;
          parseContent = parseContent.substr(pos + 1);
          continue;
        } else {
          return kBAD;
        }
      }
      case kRequestVersion: {
        auto pos = parseContent.find("\r\n");
        if (pos == npos) {
          unsolved_ = parseContent;
          return kKEEP;
        }
        auto version = parseContent.substr(0, pos);
        if (CheckVersion(version, request)) {
          state_ = kHeaderName;
          parseContent = parseContent.substr(pos + 2);
          continue;
        } else {
          return kBAD;
        }
      }
      case kHeaderName: {
        // Check Request Header complete
        auto pos = parseContent.find("\r\n");
        if (pos == 0) {  // Complete
          state_ = kBody;
          parseContent = parseContent.substr(pos + 2);
          continue;
        }
        pos = parseContent.find(":");
        if (pos == npos) {
          unsolved_ = parseContent;
          return kKEEP;
        }
        auto headerName = parseContent.substr(0, pos);
        if (CheckHeaderName(headerName, request)) {
          state_ = kHeaderValue;
          parseContent = parseContent.substr(pos + 1);
          latestHeaderName_ = headerName;
          continue;
        } else {
          return kBAD;
        }
      }
      case kHeaderValue: {
        auto pos = parseContent.find("\r\n");
        if (pos == npos) {
          unsolved_ = parseContent;
          return kKEEP;
        }
        auto headerValue = parseContent.substr(0, pos);
        if (CheckHeaderValue(latestHeaderName_, headerValue, request)) {
          state_ = kHeaderName;
          parseContent = parseContent.substr(pos + 2);
          continue;
        } else {
          return kBAD;
        }
      }
      case kBody: {
        // only support hedaer with Content-Length
        auto bodySize = GetBodySize(request);
        if (parseContent.size() < bodySize) {
          unsolved_ = parseContent;
          return kKEEP;
        }
        auto body = parseContent.substr(0, bodySize);
        if (CheckBody(body, request)) {
          state_ = kMethod;
          parseContent = parseContent.substr(bodySize);
          unsolved_ = parseContent;
          return kOK;
        } else {
          return kBAD;
        }
      }
    }
  }
}