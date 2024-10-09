#ifndef EXAMPLES_NET_SOCKS4A_RELAYTUNNEL
#define EXAMPLES_NET_SOCKS4A_RELAYTUNNEL

#include "cold/Cold.h"

/**
 * |          localSocket_           remoteSocket_
 * | client <--------------> relay <--------------->  server
 */

class Tunnel : public std::enable_shared_from_this<Tunnel> {
 public:
  Tunnel(Cold::TcpSocket localSocket, Cold::TcpSocket remoteSocket,
         int tunnelId)
      : localSocket_(std::move(localSocket)),
        remoteSocket_(std::move(remoteSocket)),
        tunnelId_(tunnelId) {}

  ~Tunnel() = default;

  Tunnel(const Tunnel&) = delete;
  Tunnel& operator=(const Tunnel&) = delete;

  void SwapData() {
    auto self = shared_from_this();
    assert(&localSocket_.GetIoContext() == &remoteSocket_.GetIoContext());
    auto& context = localSocket_.GetIoContext();
    context.CoSpawn(ClientToRelay(self));
    context.CoSpawn(RelayToClient(self));
  }

 private:
  Cold::Task<> ClientToRelay(std::shared_ptr<Tunnel> self) {
    char buf[5 * 1024 * 1024];
    while (localSocket_.CanReading()) {
      auto n = co_await localSocket_.Read(buf, sizeof buf);
      if (n <= 0) break;
      auto ret = co_await remoteSocket_.WriteN(buf, static_cast<size_t>(n));
      if (ret != static_cast<ssize_t>(n)) break;
      Cold::INFO("tunnelId: {} client -> relay {} bytes", tunnelId_, n);
    }
    Stop();
  }

  Cold::Task<> RelayToClient(std::shared_ptr<Tunnel> self) {
    char buf[5 * 1024 * 1024];
    while (remoteSocket_.CanReading()) {
      auto n = co_await remoteSocket_.Read(buf, sizeof buf);
      if (n <= 0) break;
      auto ret = co_await localSocket_.WriteN(buf, static_cast<size_t>(n));
      if (ret != static_cast<ssize_t>(n)) break;
      Cold::INFO("tunnelId: {} relay -> client {} bytes", tunnelId_, n);
    }
    Stop();
  }

  void Stop() {
    if (stoped_) return;
    Cold::INFO("tunnelId: {} stop", tunnelId_);
    localSocket_.Close();
    remoteSocket_.Close();
    stoped_ = true;
  }

  Cold::TcpSocket localSocket_;
  Cold::TcpSocket remoteSocket_;
  int tunnelId_;
  bool stoped_ = false;
};

#endif /* EXAMPLES_NET_SOCKS4A_RELAYTUNNEL */
