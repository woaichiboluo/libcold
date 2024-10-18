#define COLD_ENABLE_SSL
#include "HttpServer.h"

int main(int argc, char** argv) {
  if (argc != 3) {
    fmt::println("Usage: {} <certfile> <keyfile>", argv[0]);
    return 1;
  }
  Cold::SSLContext sslContext;
  sslContext.LoadCert(argv[1], argv[2]);
  Cold::IpAddress address(8080);
  HttpServer server(address);
  Cold::INFO("SimpleHttpsServer run at {}", address.GetIpPort());
  server.EnableSSL(sslContext);
  server.Start();
}