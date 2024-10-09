#include "RelayTunnel.h"

using namespace Cold;

class TcpRelay : public TcpServer {
 public:
  TcpRelay(const IpAddress& localAddr, const IpAddress& remoteAddr,
           size_t poolsize = 0)
      : TcpServer(localAddr, poolsize, "TcpRelay"), remoteAddr_(remoteAddr) {}
  ~TcpRelay() override = default;

 private:
  static std::atomic<int> tunnelId_;

  Task<> DoJob(TcpSocket localSocket) {
    using namespace std::chrono_literals;
    TcpSocket remoteSocket(localSocket.GetIoContext());
    auto [timeout, ret] =
        co_await Timeout(remoteSocket.Connect(remoteAddr_), 3s);
    if (timeout || !ret) {
      ERROR("failed to connect to remote: {}", remoteAddr_.GetIpPort());
      localSocket.Close();
      co_return;
    }
    ++tunnelId_;
    // data swap delegate to Tunnel
    auto tunnel = std::make_shared<Tunnel>(std::move(localSocket),
                                           std::move(remoteSocket), tunnelId_);
    tunnel->SwapData();
  }

  Task<> OnNewConnection(TcpSocket socket) override {
    INFO("new connection from {}", socket.GetRemoteAddress().GetIpPort());
    co_await DoJob(std::move(socket));
  }

  IpAddress remoteAddr_;
};

std::atomic<int> TcpRelay::tunnelId_{0};

int main(int argc, char** argv) {
  if (argc != 4) {
    fmt::println("Usage: {} <listenPort> <remoteAddr> <remotePort>", argv[0]);
    return 1;
  }
  uint16_t listenPort = static_cast<uint16_t>(std::stoi(argv[1]));
  auto addr = IpAddress::Resolve(argv[2], argv[3]);
  if (!addr) {
    fmt::println("cannot resolve remote address: {} port: {}", argv[2],
                 argv[3]);
    return 1;
  }
  TcpRelay relay(IpAddress(listenPort), *addr, 4);
  INFO("remote addr: {}", addr->GetIpPort());
  relay.Start();
}