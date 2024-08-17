#include "WebSocketEchoPage.h"
#include "cold/net/http/HttpServer.h"
#include "cold/net/http/WebSocket.h"

using namespace Cold;

class EchoWebSocketServer : public Net::Http::WebSocketServer {
 public:
  EchoWebSocketServer() = default;
  ~EchoWebSocketServer() override {
    Base::INFO("EchoWebSocketServer destructed");
  }

  void OnConnect(Net::Http::WebSocketPtr s) override {
    s->SetOnRecv([](Net::Http::WebSocket::WebSocketPtr ws, const char* data,
                    size_t len) {
      Base::INFO("On Echo recv data length: {}", len);
      ws->Send(data, len);
    });
  }

  void OnClose(Net::Http::WebSocket::WebSocketPtr ws) override {
    Base::INFO("EchoWebSocket closed");
  }
};

int main() {
  auto wsServer = std::make_unique<EchoWebSocketServer>();
  Net::Http::HttpServer httpServer(Net::IpAddress(8080));
  httpServer.AddServlet("/index.html", [](Net::Http::HttpRequest& req,
                                          Net::Http::HttpResponse& resp) {
    resp.SetStatus(Net::Http::HttpStatus::OK);
    auto body = std::make_unique<Net::Http::TextBody>();
    body->SetContent(kEchoPage);
    resp.SetBody(std::move(body));
  });
  httpServer.SetWebSocketServer(std::move(wsServer));
  httpServer.Start();
}