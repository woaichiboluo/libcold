#include "HttpServer.h"

int main() {
  Cold::IpAddress address(8080);
  HttpServer server(address);
  Cold::INFO("SimpleHttpServer run at {}", address.GetIpPort());
  server.Start();
}