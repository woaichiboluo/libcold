#include <ctime>

#include "cold/net/TcpServer.h"
#include "cold/net/ssl/SSLContext.h"

using namespace Cold;

class DaytimeServer : public Net::TcpServer {
 public:
  DaytimeServer(const Net::IpAddress& addr, size_t poolSize = 0,
                bool reusePort = false, bool enableSSL = false)
      : Net::TcpServer(addr, poolSize, reusePort, enableSSL) {}
  ~DaytimeServer() override = default;

  Base::Task<> OnConnect(Net::TcpSocket socket) override {
    Base::INFO("DayTimeServer: Connection accepted. addr: {}",
               socket.GetRemoteAddress().GetIpPort());
    time_t now = time(nullptr);
    std::string timeStr(ctime(&now));
    co_await socket.WriteN(timeStr.data(), timeStr.size());
    socket.Close();
  }
};

int main(int argc, char** argv) {
  // load the server's private key and certificate
  if (argc < 3) {
    fmt::print("Usage: {} <cert> <key>\n", argv[0]);
    return 0;
  }
  Net::SSLContext::GetInstance().LoadCert(argv[1], argv[2]);
  DaytimeServer server(Net::IpAddress(6666), 0, false, true);
  server.Start();
}