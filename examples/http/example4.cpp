#include "cold/net/http/HttpServer.h"
#include "cold/net/http/WebSocket.h"

using namespace Cold;

class EchoWebSocketServer : public Net::Http::WebSocketServer {
 public:
  EchoWebSocketServer() = default;
  ~EchoWebSocketServer() override = default;

  void OnNewWebSocket(Net::Http::WebSocketPtr s) override {
    s->SetOnRecv([](Net::Http::WebSocket::WebSocketPtr ws, const char* data,
                    size_t len) { ws->Send(data, len); });
  }
};

int main() {
  auto wsServer = std::make_unique<Net::Http::WebSocketServer>();
  Net::Http::HttpServer httpServer(Net::IpAddress(8080));
  httpServer.SetWebSocketServer(std::move(wsServer));
  httpServer.Start();
}