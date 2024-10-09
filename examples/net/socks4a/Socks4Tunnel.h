#ifndef EXAMPLES_NET_SOCKS4A_SOCKS4TUNNEL
#define EXAMPLES_NET_SOCKS4A_SOCKS4TUNNEL

#include "cold/Cold.h"

class Socks4aTunnel : public std::enable_shared_from_this<Socks4aTunnel> {
 public:
  Socks4aTunnel(Cold::TcpSocket localSocket, int tunnelId)
      : localSocket_(std::move(localSocket)), tunnelId_(tunnelId) {}

  ~Socks4aTunnel() = default;

  Socks4aTunnel(const Socks4aTunnel&) = delete;
  Socks4aTunnel& operator=(const Socks4aTunnel&) = delete;

  void SwapData() {
    auto self = shared_from_this();
    localSocket_.GetIoContext().CoSpawn(DoSocks4a(self));
  }

 private:
  enum ParseState { kOk, kNeedMore, kError };
  Cold::Task<> DoSocks4a(std::shared_ptr<Socks4aTunnel> self) {
    // static constexpr std::string_view kSuccessReply = "\000\x5aUVWXYZ";
    static constexpr std::string_view kFailedReply = "\000\x5bUVWXYZ";
    char buf[1024];
    ParseState state = kNeedMore;
    Cold::IpAddress destAddr;
    while (state == kNeedMore) {
      auto n = co_await localSocket_.Read(buf, sizeof buf);
      if (n <= 0) {
        state = kError;
      } else {
        state = ParseSocks4a(buf, buf + n, destAddr);
      }
    }
    if (state != kOk) {
      localSocket_.Close();
      co_return;
    }
    Cold::TcpSocket remoteSocket(localSocket_.GetIoContext());
    using namespace std::chrono_literals;
    Cold::TcpSocket socket(localSocket_.GetIoContext());
    auto [timeout, ret] =
        co_await Cold::Timeout(socket.Connect(destAddr), 3min);
    if (timeout || !ret) {
      co_await localSocket_.WriteN(kFailedReply.data(), kFailedReply.size());
      localSocket_.Close();
      co_return;
    }
    auto addr = *reinterpret_cast<sockaddr_in*>(destAddr.GetSockaddr());
    char response[] = "\000\x5aUVWXYZ";
    // cannot be ignored
    memcpy(response + 2, &addr.sin_port, 2);
    memcpy(response + 4, &addr.sin_addr.s_addr, 4);
    if (co_await localSocket_.WriteN(response, 8) != 8) {
      localSocket_.Close();
      co_return;
    }
    Cold::INFO("tuunelId: {} begin swap.", tunnelId_);
    remoteSocket_ = std::move(socket);
    stoped_ = false;
    localSocket_.GetIoContext().CoSpawn(ClientToRelay(self));
    localSocket_.GetIoContext().CoSpawn(RelayToClient(self));
  }

  ParseState ParseSocks4a(const char* begin, const char* end,
                          Cold::IpAddress& destAddr) {
    // | VER(1) | CMD(1) | DST_PORT(2) | DST_IP(4) | USERID | NULL |
    // | VER(1) | CMD(1) | DST_PORT(2) | DST_IP(4) | USERID | DOMAIN |
    readBuf_.insert(readBuf_.end(), begin, end);
    auto size = readBuf_.size();
    if (size < 8) return kNeedMore;
    if (size > 128) return kError;
    int8_t ver = readBuf_[0];
    int8_t cmd = readBuf_[1];
    if (ver != 4 && cmd != 1) return kError;
    auto port = Cold::ReadInt<uint16_t>(readBuf_.begin() + 2);
    auto ip = Cold::ReadInt<uint32_t>(readBuf_.begin() + 4);
    auto uid = std::find(readBuf_.begin() + 8, readBuf_.end(), '\0');
    if (uid == readBuf_.end()) return kNeedMore;
    bool socks4a = ip < 256;
    if (socks4a) {
      auto hbegin = uid + 1;
      auto hend = std::find(hbegin, readBuf_.end(), '\0');
      if (hend == readBuf_.end()) return kNeedMore;
      std::string hostname(hbegin, hend);
      // FIXME use async resolve
      Cold::INFO("tunnelId: {} begin resolve hostname: {}", tunnelId_,
                 hostname);
      auto addr =
          Cold::IpAddress::Resolve(hostname, fmt::format("{}", port).data());
      Cold::INFO("end tunnelId: {} of resolve hostname: {}", tunnelId_,
                 hostname);
      if (!addr) return kError;
      destAddr = *addr;
      readBuf_.erase(readBuf_.begin(), hend + 1);
    } else {
      struct sockaddr_in addr;
      addr.sin_family = AF_INET;
      addr.sin_port = Cold::Endian::Host16ToNetwork16(port);
      addr.sin_addr.s_addr = Cold::Endian::Host32ToNetwork32(ip);
      destAddr = Cold::IpAddress(addr);
      readBuf_.erase(readBuf_.begin(), uid + 1);
    }
    return kOk;
  }

  // client <-> relay <-> server
  Cold::Task<> ClientToRelay(std::shared_ptr<Socks4aTunnel> self) {
    char buf[5 * 1024 * 1024];
    assert(readBuf_.size() < sizeof buf);
    std::copy(readBuf_.begin(), readBuf_.end(), buf);
    readBuf_.clear();
    while (localSocket_.CanReading()) {
      char* begin = buf + readBuf_.size();
      auto size = static_cast<size_t>((buf + sizeof buf) - begin);
      auto n = co_await localSocket_.Read(begin, size);
      if (n <= 0) break;
      auto ret = co_await remoteSocket_.WriteN(buf, static_cast<size_t>(n));
      if (ret != static_cast<ssize_t>(n)) break;
    }
    Stop();
  }

  Cold::Task<> RelayToClient(std::shared_ptr<Socks4aTunnel> self) {
    char buf[5 * 1024 * 1024];
    while (remoteSocket_.CanReading()) {
      auto n = co_await remoteSocket_.Read(buf, sizeof buf);
      if (n <= 0) break;
      auto ret = co_await localSocket_.WriteN(buf, static_cast<size_t>(n));
      if (ret != static_cast<ssize_t>(n)) break;
    }
    Stop();
  }

  void Stop() {
    if (stoped_) return;
    localSocket_.Close();
    remoteSocket_.Close();
    Cold::INFO("tunnelId: {} stop", tunnelId_);
  }

  Cold::TcpSocket localSocket_;
  int tunnelId_;
  Cold::TcpSocket remoteSocket_;
  std::vector<char> readBuf_;
  bool stoped_ = true;
};

#endif /* EXAMPLES_NET_SOCKS4A_SOCKS4TUNNEL */
