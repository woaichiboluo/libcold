#ifndef EXAMPLES_NET_SIMPLEHTTP_HTTPRESPONSE
#define EXAMPLES_NET_SIMPLEHTTP_HTTPRESPONSE

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "HttpRequest.h"

class HttpRequest;

class HttpResponse {
 public:
  HttpResponse() = default;
  ~HttpResponse();

  HttpResponse(const HttpResponse&) = delete;
  HttpResponse& operator=(const HttpResponse&) = delete;

  static void MakeResponse(bool ok, HttpRequest& request,
                           HttpResponse& response);

  void Reset();

  bool HasBody() const { return mmapAddr_.has_value(); }
  const std::vector<char>& GetHeaders() const { return buf_; }
  std::pair<void*, size_t> GetMmapAddr() const { return mmapAddr_.value(); }

 private:
  void MakeResponseHeaders(int status, std::string_view version);

  std::map<std::string, std::string> headers_;
  std::vector<char> buf_;
  std::optional<std::pair<void*, size_t>> mmapAddr_;
};

#endif /* EXAMPLES_NET_SIMPLEHTTP_HTTPRESPONSE */
