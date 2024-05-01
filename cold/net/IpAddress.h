#ifndef COLD_NET_IPADDRESS
#define COLD_NET_IPADDRESS

#include <arpa/inet.h>
#include <sys/un.h>

#include <optional>
#include <string_view>

namespace Cold::Net {

class IpAddress {
 public:
  IpAddress() : IpAddress(0, false, false) {}

  IpAddress(std::string_view address, uint16_t port, bool ipv6 = false);
  explicit IpAddress(uint16_t port, bool loopback = false, bool ipv6 = false);
  explicit IpAddress(const struct sockaddr_in& addr) : ipv4Addr_(addr) {}
  explicit IpAddress(const struct sockaddr_in6& addr) : ipv6Addr_(addr) {}

  const struct sockaddr* GetSockaddr() const {
    return reinterpret_cast<const struct sockaddr*>(&ipv6Addr_);
  }

  struct sockaddr* GetSockaddr() {
    return reinterpret_cast<struct sockaddr*>(&ipv6Addr_);
  }

  std::string GetIp() const;
  uint16_t GetPort() const;
  std::string GetIpPort() const;

  sa_family_t GetFamily() const { return ipv4Addr_.sin_family; }

  bool IsIpv4() const { return ipv4Addr_.sin_family == AF_INET; }
  bool IsIpv6() const { return ipv6Addr_.sin6_family == AF_INET6; }

  void SetScopeId(uint32_t scope_id);

  static std::optional<IpAddress> Resolve(std::string_view hostname,
                                          const char* service,
                                          bool ipv6 = false);

 private:
  union {
    struct sockaddr_in ipv4Addr_;
    struct sockaddr_in6 ipv6Addr_ {};
  };
};

}  // namespace Cold::Net

#endif /* COLD_NET_IPADDRESS */
