#include "cold/net/http/HttpServer.h"

using namespace Cold;

using namespace Cold::Net::Http;

int main() {
  Net::IpAddress addr(8080);
  HttpServer server(addr, 4);
  server.AddServlet("/hello", [](HttpRequest& request, HttpResponse& response) {
    auto body = MakeTextBody();
    body->SetContent("<h1>Hello World</h1>");
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
  Base::INFO("example1:basic servlet usage. Run at port: 8080");
  server.Start();
}