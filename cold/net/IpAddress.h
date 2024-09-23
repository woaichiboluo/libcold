#ifndef COLD_NET_IPADDRESS
#define COLD_NET_IPADDRESS

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/un.h>

#include <cassert>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "../Log.h"
#include "../util/ScopeUtil.h"
#include "Endian.h"

namespace Cold {

class IpAddress {
 public:
  IpAddress() : IpAddress(0, false, false) {}

  IpAddress(std::string_view address, uint16_t port, bool ipv6 = false) {
    int ret = 0;
    if (address[0] == '[' && address.back() == ']') {
      address.remove_prefix(1);
      address.remove_suffix(1);
    }
    char buf[256] = {};
    memcpy(buf, address.data(), address.size());
    if (ipv6) {
      ipv6Addr_.sin6_family = AF_INET6;
      ret = inet_pton(AF_INET6, buf, &ipv6Addr_.sin6_addr);
      ipv6Addr_.sin6_port = Endian::Host16ToNetwork16(port);
    } else {
      ipv4Addr_.sin_family = AF_INET;
      ret = inet_pton(AF_INET, buf, &ipv4Addr_.sin_addr.s_addr);
      ipv4Addr_.sin_port = Endian::Host16ToNetwork16(port);
    }
    assert(ret == 1);
    (void)ret;
  }

  explicit IpAddress(uint16_t port, bool loopback = false, bool ipv6 = false) {
    if (ipv6) {
      ipv6Addr_.sin6_family = AF_INET6;
      ipv6Addr_.sin6_addr = loopback ? in6addr_loopback : in6addr_any;
      ipv6Addr_.sin6_port = Endian::Host16ToNetwork16(port);
    } else {
      ipv4Addr_.sin_family = AF_INET;
      ipv4Addr_.sin_addr.s_addr =
          loopback ? Endian::Host32ToNetwork32(INADDR_LOOPBACK) : INADDR_ANY;
      ipv4Addr_.sin_port = Endian::Host16ToNetwork16(port);
    }
  }

  explicit IpAddress(const struct sockaddr_in& addr) : ipv4Addr_(addr) {}
  explicit IpAddress(const struct sockaddr_in6& addr) : ipv6Addr_(addr) {}

  ~IpAddress() = default;

  bool IsIpv4() const { return ipv4Addr_.sin_family == AF_INET; }
  bool IsIpv6() const { return ipv6Addr_.sin6_family == AF_INET6; }

  sa_family_t GetFamily() const { return ipv4Addr_.sin_family; }

  const struct sockaddr* GetSockaddr() const {
    return reinterpret_cast<const struct sockaddr*>(&ipv6Addr_);
  }

  socklen_t GetSocklen() const {
    return IsIpv4() ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6);
  }

  struct sockaddr* GetSockaddr() {
    return reinterpret_cast<struct sockaddr*>(&ipv6Addr_);
  }

  std::string GetIp() const {
    char buf[256] = {'['};
    if (GetFamily() == AF_INET) {
      inet_ntop(AF_INET, &ipv4Addr_.sin_addr.s_addr, buf, sizeof buf);
      return buf;
    } else {
      inet_ntop(AF_INET6, &ipv6Addr_.sin6_addr, buf + 1, sizeof(buf) - 1);
      std::string ret(buf);
      ret.push_back(']');
      return ret;
    }
  }

  uint16_t GetPort() const {
    return Endian::Network16ToHost16(ipv6Addr_.sin6_port);
  }

  std::string GetIpPort() const {
    return fmt::format("{}:{}", GetIp(), GetPort());
  }

  void SetScopeId(uint32_t scope_id) {
    if (!IsIpv6()) return;
    ipv6Addr_.sin6_scope_id = scope_id;
  }

  static std::optional<IpAddress> Resolve(std::string_view hostname,
                                          const char* serviceOrPort,
                                          bool ipv6 = false,
                                          bool onlytcp = false) {
    struct addrinfo hints {};
    struct addrinfo* res{};
    hints.ai_family = ipv6 ? AF_INET6 : AF_INET;
    hints.ai_socktype = onlytcp ? SOCK_STREAM : 0;
    if (getaddrinfo(hostname.data(), serviceOrPort, &hints, &res) != 0) {
      ERROR("Resolve error hostname: {}. reason: {}", hostname,
            gai_strerror(errno));
      return {};
    }
    assert(res);
    ScopeGuard guard([=]() { freeaddrinfo(res); });
    if (ipv6) {
      return {IpAddress(*reinterpret_cast<struct sockaddr_in6*>(res->ai_addr))};
    }
    return {IpAddress(*reinterpret_cast<struct sockaddr_in*>(res->ai_addr))};
  }

  static std::vector<IpAddress> ResolveAll(std::string_view hostname,
                                           const char* serviceOrPort,
                                           bool onlyipv4 = false,
                                           bool onlytcp = false) {
    struct addrinfo hints {};
    struct addrinfo* res{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = onlytcp ? SOCK_STREAM : 0;
    if (getaddrinfo(hostname.data(), serviceOrPort, &hints, &res) != 0) {
      ERROR("Resolve error hostname: {}. reason: {}", hostname,
            gai_strerror(errno));
      return {};
    }
    assert(res);
    ScopeGuard guard([=]() { freeaddrinfo(res); });
    std::vector<IpAddress> addrs;
    for (auto begin = res; begin; begin = begin->ai_next) {
      if (begin->ai_family == AF_INET) {
        addrs.push_back(
            IpAddress(*reinterpret_cast<struct sockaddr_in*>(begin->ai_addr)));
      } else if (!onlyipv4) {
        addrs.push_back(
            IpAddress(*reinterpret_cast<struct sockaddr_in6*>(begin->ai_addr)));
      }
    }
    return addrs;
  }

 private:
  union {
    struct sockaddr_in ipv4Addr_;
    struct sockaddr_in6 ipv6Addr_ {};
  };
};

}  // namespace Cold

#endif /* COLD_NET_IPADDRESS */
