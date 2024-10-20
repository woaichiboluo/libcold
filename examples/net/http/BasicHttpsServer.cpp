#define COLD_ENABLE_SSL
#include "cold/Cold-Http.h"

using namespace Cold;
using namespace Cold::Http;

int main(int argc, char** argv) {
  if (argc != 3) {
    fmt::println("Usage: {} <cert> <key>", argv[0]);
    return 1;
  }
  SSLContext sslContext;
  sslContext.LoadCert(argv[1], argv[2]);
  IpAddress addr(8080);
  HttpServer server(addr);
  server.EnableSSL(sslContext);
  server.SetHost("localhost");
  server.AddServlet("/hello", [](HttpRequest& request, HttpResponse& response) {
    auto body = MakeHttpBody<HtmlTextBody>();
    body->Append("<h1>Hello World</h1>");
    response.SetBody(std::move(body));
  });
  server.AddServlet("/hello1",
                    [](HttpRequest& request, HttpResponse& response) {
                      response.SendRedirect("/hello");
                    });
  server.AddServlet(
      "/hello2", [](HttpRequest& request, HttpResponse& response) {
        request.GetServletContext()->ForwardTo("/hello", request, response);
      });
  server.AddServlet("/donothing",
                    [](HttpRequest& request, HttpResponse& response) {});
  server.AddServlet("/**", std::make_unique<StaticFileServlet>("."));
  INFO("basic https server . Run at port: 8080");
  server.Start();
  return 0;
}