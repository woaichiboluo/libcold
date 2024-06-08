#include "cold/net/http/HttpServer.h"

#include "cold/net/http/HttpRequestParser.h"

using namespace Cold;

Base::Task<> Net::Http::HttpServer::DoHttp(Net::TcpSocket socket) {
  HttpRequestParser parser;
  std::vector<char> headerBuf;
  char buf[65536];
  while (true) {
    HttpResponse response;
    auto n = co_await socket.ReadWithTimeout(buf, sizeof buf,
                                             std::chrono::seconds(15));
    if (n <= 0) {
      socket.Close();
      co_return;
    }
    if (!parser.Parse(buf, static_cast<size_t>(n))) {  // Bad Request
      auto body = std::make_unique<TextBody>();
      body->SetContent("<h1>400 BAD_REQUEST</h1>");
      response.SetStatus(HttpStatus::BAD_REQUEST);
      response.SetCloseConnection(true);
      response.SetBody(std::move(body));
    } else if (parser.HasRequest()) {
      auto request = parser.TakeRequest();
      request.DecodeUrlAndBody();
      request.SetServletContext(&context_);
      response.SetCloseConnection(!request.IsKeepAlive());
      response.SetVersion(std::string(request.GetVersion()));
      context_.ForwardTo(request.GetUrl(), request, response);
    } else {
      continue;
    }
    // Send response
    headerBuf.clear();
    response.MakeHeaders(headerBuf);
    if (co_await socket.WriteN(headerBuf.data(), headerBuf.size()) <= 0) {
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