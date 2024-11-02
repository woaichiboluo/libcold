#define COLD_ENABLE_SSL
#include "WebSocketEchoPage.h"
#include "cold/Cold-Http.h"

using namespace Cold;

int main() {
  auto wsServer = std::make_unique<Http::WebSocketServer>("/**");
  wsServer->SetOnConnect([](Http::WebSocketPtr s) {
    s->SetOnMessage([](Http::WebSocketPtr ws, std::string_view data) {
      INFO("received data: {}", data);
      ws->Send(data);
    });
  });

  Http::HttpServer httpServer(IpAddress(8080), 4);
  httpServer.AddServlet("/index.html",
                        [](Http::HttpRequest& req, Http::HttpResponse& resp) {
                          auto body = Http::MakeHttpBody<Http::HtmlTextBody>();
                          body->Append(kWsEchoPage);
                          resp.SetBody(std::move(body));
                        });
  httpServer.AddWsServer(std::move(wsServer));
  INFO("Echo WebSocket Server. Run at port: 8080");
  httpServer.Start();
}