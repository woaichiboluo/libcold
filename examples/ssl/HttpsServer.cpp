#include "cold/net/http/HttpServer.h"
#include "cold/net/http/WebSocketServer.h"
#include "cold/net/ssl/SSLContext.h"

using namespace Cold;

using namespace Cold::Net::Http;

class HelloWebSocket : public WebSocketServer {
 public:
  HelloWebSocket() = default;
  ~HelloWebSocket() override = default;

  void OnConnect(WebSocketPtr ws) override {
    ws->SetOnRecv([ws](WebSocketPtr, const char* data, size_t len) {
      std::string_view sv("hello websocket");
      ws->Send(sv.data(), sv.size());
    });
  }
  void OnClose(WebSocketPtr ws) override { Base::INFO("WebSocket closed"); }
};

int main(int argc, char** argv) {
  if (argc < 3) {
    fmt::print("Usage: {} <cert> <key>\n", argv[0]);
    return 0;
  }
  Net::IpAddress addr(8080);
  Net::SSLContext::GetInstance().LoadCert(argv[1], argv[2]);
  HttpServer server(addr, 4, false, true);
  auto wsServer = std::make_unique<HelloWebSocket>();
  server.SetWebSocketServer(std::move(wsServer));
  Base::INFO("https server usage. Run at port: 8080");
  server.Start();
}
