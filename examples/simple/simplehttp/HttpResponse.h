#ifndef SIMPLE_SIMPLEHTTP_HTTPRESPONSE
#define SIMPLE_SIMPLEHTTP_HTTPRESPONSE

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "cold/net/TcpSocket.h"
#include "examples/simple/simplehttp/HttpRequest.h"

class HttpRequest;

class HttpResponse {
 public:
  HttpResponse() = default;
  ~HttpResponse();

  HttpResponse(const HttpResponse&) = delete;
  HttpResponse& operator=(const HttpResponse&) = delete;

  static void MakeResponse(bool ok, HttpRequest& request,
                           HttpResponse& response);

  Cold::Base::Task<ssize_t> SendRequest(Cold::Net::TcpSocket& socket);

 private:
  void MakeResponseHeaders(int status, std::string_view version);

  std::map<std::string, std::string> headers_;
  std::vector<char> buf_;
  std::optional<std::pair<void*, size_t>> mmapAddr_;
};

#endif /* SIMPLE_SIMPLEHTTP_HTTPRESPONSE */
