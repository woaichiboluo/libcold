#include "cold/net/http/WebSocketServer.h"

#include "cold/net/http/HttpRequestParser.h"
#include "cold/net/http/HttpResponse.h"
#include "cold/net/http/WebSocket.h"
#include "cold/util/Crypto.h"

using namespace Cold;

Base::Task<> Net::Http::WebSocketServer::StandAloneWsServerContext::OnConnect(
    TcpSocket socket) {
  static const int kReadTimeoutMs =
      Base::Config::GetGloablDefaultConfig().GetOrDefault(
          "/http/read-timeout-ms", 15000);
  char buf[65536];
  while (true) {
    auto n = co_await socket.ReadWithTimeout(
        buf, sizeof buf, std::chrono::milliseconds(kReadTimeoutMs));
    if (n <= 0) {
      socket.Close();
      co_return;
    }
    HttpRequestParser parser;
    if (!parser.Parse(buf, static_cast<size_t>(n))) {
      socket.Close();
    } else if (parser.HasRequest()) {
      auto rawRequest = parser.TakeRequest();
      onUpgradeRequest_(rawRequest, std::move(socket));
    }
  }
}

bool Net::Http::WebSocketServer::CheckWhetherUpgradeRequest(
    RawHttpRequest request) const {
  return request.GetHeader("Connection") == "Upgrade" &&
         request.GetHeader("Upgrade") == "websocket" &&
         request.HasHeader("Sec-WebSocket-Key");
}

Net::Http::WebSocketServer::WebSocketServer(const Net::IpAddress& addr,
                                            size_t poolSize, bool reusePort,
                                            bool enableSSL)
    : wsServer_(std::make_unique<StandAloneWsServerContext>(
          addr, poolSize, reusePort, enableSSL)) {
  wsServer_->onUpgradeRequest_ =
      std::bind(&WebSocketServer::OnReceivedUpgradeRequest, this,
                std::placeholders::_1, std::placeholders::_2);
}

Base::Task<> Net::Http::WebSocketServer::OnReceivedUpgradeRequest(
    RawHttpRequest request, TcpSocket socket) {
  auto websocketKey = request.GetHeader("Sec-WebSocket-Key");
  auto key = std::string(websocketKey) + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  auto encodedKey = Base::Base64Encode(Base::SHA1(key));
  HttpResponse response;
  response.SetStatus(HttpStatus::SWITCHING_PROTOCOLS);
  response.SetUpgradeConnection();
  response.SetHeader("Upgrade", "websocket");
  response.SetHeader("Sec-WebSocket-Accept", encodedKey);
  std::string headerBuf;
  response.MakeHeaders(headerBuf);
  if (co_await socket.WriteN(headerBuf.data(), headerBuf.size()) !=
      static_cast<ssize_t>(headerBuf.size())) {
    socket.Close();
    co_return;
  }

  // handshake complete
  // cannot discard the url here, because it may contain query string
  auto url = UrlDecode(request.GetUrl());
  auto ws = std::make_shared<WebSocket>(std::move(url), std::move(socket));
  ws->onClose_ = [this](std::shared_ptr<WebSocket> sock) {
    Base::LockGuard guard(mutex_);
    webSockets_.erase(sock);
    OnClose(sock);
  };
  {
    Base::LockGuard guard(mutex_);
    webSockets_.insert(ws);
  }
  OnConnect(ws);
  co_await ws->DoRead();
}