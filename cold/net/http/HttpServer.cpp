#include "cold/net/http/HttpServer.h"

#include "cold/net/http/HttpRequestParser.h"
#include "cold/util/Config.h"

using namespace Cold;

Base::Task<> Net::Http::HttpServer::DoHttp(Net::TcpSocket socket) {
  HttpRequestParser parser;
  static const int kReadTimeoutMs =
      Base::Config::GetGloablDefaultConfig().GetOrDefault(
          "/http/read-timeout-ms", 15000);
  static const int kWriteTimeoutMs =
      Base::Config::GetGloablDefaultConfig().GetOrDefault(
          "/http/write-timeout-ms", 15000);
  std::string headerBuf;
  char buf[65536];
  while (true) {
    HttpResponse response;
    auto n = co_await socket.ReadWithTimeout(
        buf, sizeof buf, std::chrono::milliseconds(kReadTimeoutMs));
    if (n <= 0) {
      socket.Close();
      co_return;
    }
    if (!parser.Parse(buf, static_cast<size_t>(n))) {  // Bad Request
      assert(badRequestBodyCall_);
      auto body = badRequestBodyCall_();
      response.SetStatus(HttpStatus::BAD_REQUEST);
      response.SetCloseConnection(true);
      response.SetBody(std::move(body));
    } else if (parser.HasRequest()) {
      auto rawRequest = parser.TakeRequest();
      HttpRequest request(rawRequest, &response, &context_);
      response.SetCloseConnection(!request.IsKeepAlive());
      response.SetVersion(std::string(request.GetVersion()));
      context_.ForwardTo(request.GetUrl(), request, response);
    } else {
      continue;
    }
    // Send response
    headerBuf.clear();
    response.MakeHeaders(headerBuf);
    if (co_await socket.WriteNWithTimeout(
            headerBuf.data(), headerBuf.size(),
            std::chrono::milliseconds(kWriteTimeoutMs)) <= 0) {
      socket.Close();
      co_return;
    }
    if (!co_await response.SendBody(socket)) {
      socket.Close();
      co_return;
    }
    if (!response.IsKeepAlive()) {
      socket.Close();
      co_return;
    }
  }
}