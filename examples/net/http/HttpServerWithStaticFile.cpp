#include "cold/Cold-Http.h"

using namespace Cold;
using namespace Cold::Http;

int main() {
  IpAddress addr(8080);
  HttpServer server(addr, 4);
  server.SetHost("localhost");
  server.AddServlet("/**", std::make_unique<StaticFileServlet>("."));
  INFO("http server with static file. Run at port: 8080");
  server.Start();
  return 0;
}
