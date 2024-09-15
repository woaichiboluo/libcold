#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cold/Net.h"
#include "third_party/doctest.h"

using namespace Cold;

TEST_CASE("test ipv4 address") {
  IpAddress addr;
  CHECK(addr.GetFamily() == AF_INET);
  CHECK(addr.IsIpv4());
  CHECK(!addr.IsIpv6());
  CHECK(addr.GetIp() == "0.0.0.0");
  CHECK(addr.GetPort() == 0);
  CHECK(addr.GetIpPort() == "0.0.0.0:0");

  IpAddress addr1(80, true);
  CHECK(addr1.GetFamily() == AF_INET);
  CHECK(addr1.IsIpv4());
  CHECK(!addr1.IsIpv6());
  CHECK(addr1.GetIp() == "127.0.0.1");
  CHECK(addr1.GetPort() == 80);
  CHECK(addr1.GetIpPort() == "127.0.0.1:80");

  IpAddress addr2("192.168.0.1", 443);
  CHECK(addr2.GetFamily() == AF_INET);
  CHECK(addr2.IsIpv4());
  CHECK(!addr2.IsIpv6());
  CHECK(addr2.GetIp() == "192.168.0.1");
  CHECK(addr2.GetPort() == 443);
  CHECK(addr2.GetIpPort() == "192.168.0.1:443");
}

TEST_CASE("test ipv6 address") {
  IpAddress addr(80, false, true);
  CHECK(addr.GetFamily() == AF_INET6);
  CHECK(!addr.IsIpv4());
  CHECK(addr.IsIpv6());
  CHECK(addr.GetIp() == "[::]");
  CHECK(addr.GetPort() == 80);
  CHECK(addr.GetIpPort() == "[::]:80");

  IpAddress addr1(8080, true, true);
  CHECK(addr1.GetFamily() == AF_INET6);
  CHECK(!addr1.IsIpv4());
  CHECK(addr1.IsIpv6());
  CHECK(addr1.GetIp() == "[::1]");
  CHECK(addr1.GetPort() == 8080);
  CHECK(addr1.GetIpPort() == "[::1]:8080");

  IpAddress addr2("[1::111]", 443, true);
  CHECK(addr2.GetFamily() == AF_INET6);
  CHECK(!addr2.IsIpv4());
  CHECK(addr2.IsIpv6());
  CHECK(addr2.GetIp() == "[1::111]");
  CHECK(addr2.GetPort() == 443);
  CHECK(addr2.GetIpPort() == "[1::111]:443");
}

TEST_CASE("test resolve") {
  auto addr = IpAddress::Resolve("www.baidu.com", "80");
  CHECK(addr);
  CHECK(addr->GetFamily() == AF_INET);
  CHECK(addr->IsIpv4());
  CHECK(!addr->IsIpv6());
  CHECK(addr->GetPort() == 80);

  auto addr1 = IpAddress::Resolve("www.baidu.com", "443", true, true);
  CHECK(addr1.has_value());
  CHECK(addr1->GetFamily() == AF_INET6);
  CHECK(!addr1->IsIpv4());
  CHECK(addr1->IsIpv6());
  CHECK(addr1->GetPort() == 443);

  auto addrs = IpAddress::ResolveAll("www.baidu.com", "80", true, true);
  CHECK(!addrs.empty());
  for (const auto& a : addrs) {
    CHECK(a.GetFamily() == AF_INET);
    CHECK(a.IsIpv4());
    CHECK(!a.IsIpv6());
    CHECK(a.GetPort() == 80);
  }

  auto addrs1 = IpAddress::ResolveAll("www.baidu.com", "80", false, true);
  CHECK(!addrs1.empty());
  for (const auto& a : addrs1) {
    CHECK(a.GetPort() == 80);
  }

  CHECK(addrs1.size() > addrs.size());
}
