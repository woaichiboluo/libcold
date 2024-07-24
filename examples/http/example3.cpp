#include "cold/net/http/HttpServer.h"
#include "cold/net/http/StaticFileServlet.h"

using namespace Cold;

using namespace Cold::Net::Http;

int main() {
  Net::IpAddress addr(8080);
  HttpServer server(addr, 4);
  auto servlet = std::make_unique<StaticFileServlet>(".");
  server.SetDefaultServlet(std::move(servlet));
  Base::INFO("example3:basic staticfile usage. Run at port: 8080");
  server.Start();
}