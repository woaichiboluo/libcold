#define COLD_ENABLE_SSL
#include "WebSocketEchoPage.h"
#include "cold/Cold-Http.h"

using namespace Cold;

int main(int argc, char** argv) {
  if (argc < 3) {
    fmt::println("Usage: {} <cert> <key>", argv[0]);
    return 1;
  }

  auto wsServer = std::make_unique<Http::WebSocketServer>("/**");

  wsServer->SetOnConnect([](Http::WebSocketPtr s) {
    s->SetOnMessage([](Http::WebSocketPtr ws, std::string_view data) {
      INFO("received data: {}", data);
      ws->Send(data);
    });
  });

  SSLContext sslContext;
  sslContext.LoadCert(argv[1], argv[2]);

  Http::HttpServer httpServer(IpAddress(8080), 4);
  httpServer.EnableSSL(sslContext);
  httpServer.AddServlet("/index.html",
                        [](Http::HttpRequest& req, Http::HttpResponse& resp) {
                          auto body = Http::MakeHttpBody<Http::HtmlTextBody>();
                          body->Append(kWssEchoPage);
                          resp.SetBody(std::move(body));
                        });
  httpServer.AddWsServer(std::move(wsServer));
  INFO("Echo WebSockets Server. Run at port: 8080");
  httpServer.Start();
}