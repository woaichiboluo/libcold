#include "cold/Cold-Http.h"

using namespace Cold;
using namespace Cold::Http;

int main() {
  IpAddress addr(8080);
  HttpServer server(addr, 4);
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
  INFO("basic httpserver . Run at port: 8080");
  server.Start();
  return 0;
}
