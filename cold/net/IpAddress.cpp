#include "cold/net/IpAddress.h"

#include <netdb.h>

#include <cassert>

#include "cold/log/Logger.h"
#include "cold/net/Endian.h"
#include "cold/util/ScopeUtil.h"

using namespace Cold::Net;

IpAddress::IpAddress(std::string_view address, uint16_t port, bool ipv6) {
  int ret = 0;
  if (ipv6) {
    ipv6Addr_.sin6_family = AF_INET6;
    ret = inet_pton(AF_INET6, address.data(), &ipv6Addr_.sin6_addr);
    ipv6Addr_.sin6_port = Host16ToNetwork16(port);
  } else {
    ipv4Addr_.sin_family = AF_INET;
    ret = inet_pton(AF_INET, address.data(), &ipv4Addr_.sin_addr.s_addr);
    ipv4Addr_.sin_port = Host16ToNetwork16(port);
  }
  assert(ret == 1);
}

IpAddress::IpAddress(uint16_t port, bool loopback, bool ipv6) {
  if (ipv6) {
    ipv6Addr_.sin6_family = AF_INET6;
    ipv6Addr_.sin6_addr = loopback ? in6addr_loopback : in6addr_any;
    ipv6Addr_.sin6_port = Host16ToNetwork16(port);
  } else {
    ipv4Addr_.sin_family = AF_INET;
    ipv4Addr_.sin_addr.s_addr =
        loopback ? Host32ToNetwork32(INADDR_LOOPBACK) : INADDR_ANY;
    ipv4Addr_.sin_port = Host16ToNetwork16(port);
  }
}

void IpAddress::SetScopeId(uint32_t scope_id) {
  if (IsIpv6()) {
    ipv6Addr_.sin6_scope_id = scope_id;
  }
}

std::string IpAddress::GetIp() const {
  char buf[256] = {'['};
  const char* end = nullptr;
  if (GetFamily() == AF_INET) {
    end = inet_ntop(AF_INET, &ipv4Addr_.sin_addr.s_addr, buf, sizeof buf);
    assert(end != nullptr);
    return buf;
  } else {
    end = inet_ntop(AF_INET6, &ipv6Addr_.sin6_addr, buf + 1, sizeof(buf) - 1);
    assert(end != nullptr);
    std::string ret(buf);
    ret.push_back(']');
    return ret;
  }
}

uint16_t IpAddress::GetPort() const {
  return Network16ToHost16(ipv6Addr_.sin6_port);
}

std::string IpAddress::GetIpPort() const {
  return fmt::format("{}:{}", GetIp(), GetPort());
}

std::optional<IpAddress> IpAddress::Resolve(std::string_view hostname,
                                            bool ipv6) {
  struct addrinfo hints {};
  struct addrinfo* res{};
  hints.ai_family = ipv6 ? AF_INET6 : AF_INET;
  hints.ai_socktype = SOCK_DGRAM;
  if (getaddrinfo(hostname.data(), nullptr, &hints, &res) != 0) {
    LOG_ERROR(Base::GetMainLogger(), "Resolve hostname:{} error. Reason:{}",
              hostname, gai_strerror(errno));
    return {};
  }
  assert(res);
  Base::ScopeGuard guard([=]() { freeaddrinfo(res); });
  if (ipv6) {
    return {IpAddress(*reinterpret_cast<struct sockaddr_in6*>(res->ai_addr))};
  }
  return {IpAddress(*reinterpret_cast<struct sockaddr_in*>(res->ai_addr))};
}