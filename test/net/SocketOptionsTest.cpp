#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "cold/net/SocketOptions.h"
#include "cold/net/TcpSocket.h"
#include "third_party/doctest.h"

using namespace Cold;

TEST_CASE("test socket options") {
  Base::IoService service;
  Net::TcpSocket socket(service);
  // Set
  CHECK(socket.SetOption(Net::SocketOptions::ReuseAddress(true)));
  CHECK(socket.SetOption(Net::SocketOptions::ReusePort(true)));
  CHECK(socket.SetOption(Net::SocketOptions::TcpNoDelay(true)));
  CHECK(socket.SetOption(Net::SocketOptions::KeepAlive(true)));
  CHECK(socket.SetOption(Net::SocketOptions::Linger(true, 3)));
  // Get
  Net::SocketOptions::ReuseAddress ReuseAddress;
  CHECK(ReuseAddress.value == 0);
  CHECK(socket.GetOption(ReuseAddress));
  CHECK(ReuseAddress.value == 1);
  Net::SocketOptions::ReusePort ReusePort;
  CHECK(ReusePort.value == 0);
  CHECK(socket.GetOption(ReusePort));
  CHECK(ReusePort.value == 1);
  Net::SocketOptions::TcpNoDelay tcpNoDelay;
  CHECK(tcpNoDelay.value == 0);
  CHECK(socket.GetOption(tcpNoDelay));
  CHECK(tcpNoDelay.value == 1);
  Net::SocketOptions::KeepAlive keepAlive;
  CHECK(keepAlive.value == 0);
  CHECK(socket.GetOption(keepAlive));
  CHECK(keepAlive.value == 1);
  Net::SocketOptions::Linger linger;
  CHECK(linger.value.l_onoff == 0);
  CHECK(linger.value.l_linger == 0);
  CHECK(socket.GetOption(linger));
  CHECK(linger.value.l_onoff == 1);
  CHECK(linger.value.l_linger == 3);
}