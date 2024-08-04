#include "cold/net/http/HttpServer.h"
#include "cold/net/ssl/SSLContext.h"

using namespace Cold;

using namespace Cold::Net::Http;

int main(int argc, char** argv) {
  if (argc < 3) {
    fmt::print("Usage: {} <cert> <key>\n", argv[0]);
    return 0;
  }
  Net::IpAddress addr(8080);
  Net::SSLContext::GetInstance().LoadCert(argv[1], argv[2]);
  HttpServer server(addr, 4, false, true);
  Base::INFO("https server usage. Run at port: 8080");
  server.Start();
}
