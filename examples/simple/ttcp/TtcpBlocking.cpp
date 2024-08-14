#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include "cold/log/Logger.h"
#include "cold/net/IpAddress.h"
#include "cold/util/Config.h"
#include "cold/util/ScopeUtil.h"
#include "examples/simple/ttcp/TtcpCommon.h"

using namespace Cold;

int AcceptOrDie(uint16_t port) {
  int listensock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  Base::ScopeGuard guard([=]() {
    if (listensock >= 0) close(listensock);
  });
  if (listensock < 0) {
    perror("Cannot create listening socket");
    exit(-1);
  }

  int flag = 1;
  if (setsockopt(listensock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) <
      0) {
    perror("Cannot set reuse address");
    exit(-1);
  }

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(listensock, reinterpret_cast<struct sockaddr*>(&addr),
           sizeof(addr)) < 0) {
    perror("Cannot bind to the listening socket");
    exit(-1);
  }

  if (listen(listensock, 10) < 0) {
    perror("Cannot listen on the listening socket");
    exit(-1);
  }

  int acceptsock = accept(listensock, nullptr, nullptr);
  if (acceptsock < 0) {
    perror("Cannot accept on the listening socket");
    exit(-1);
  }

  return acceptsock;
}

int ReadN(int fd, void* buf, size_t n) {
  size_t already = 0;
  while (already < n) {
    auto ret = read(fd, static_cast<char*>(buf) + already, n - already);
    if (ret > 0)
      already += static_cast<size_t>(ret);
    else if (ret == 0)
      break;
    else if (errno != EINTR) {
      perror("Cannot read from socket");
      exit(-1);
    }
  }
  return static_cast<int>(already);
}

int WriteN(int fd, void* buf, size_t n) {
  size_t already = 0;
  while (already < n) {
    auto ret = write(fd, static_cast<char*>(buf) + already, n - already);
    if (ret > 0)
      already += static_cast<size_t>(ret);
    else if (ret == 0)
      break;
    else if (errno != EINTR) {
      perror("Cannot write to socket");
      exit(-1);
    }
  }
  return static_cast<int>(already);
}

void Receiver() {
  auto port = Base::Config::GetGloablDefaultConfig().GetOrDefault<uint16_t>(
      "/ttcp/server-port", 6666);
  auto tcpNodelay = Base::Config::GetGloablDefaultConfig().GetOrDefault<bool>(
      "/ttcp/tcpNoDelay", false);
  Base::INFO("Start receiving. port : {}, tcpNoDelay : {}", port, tcpNodelay);
  int acceptsock = AcceptOrDie(port);
  Base::ScopeGuard guard([=]() { close(acceptsock); });
  if (tcpNodelay) {
    int flag = 1;
    if (setsockopt(acceptsock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) <
        0) {
      perror("Cannot set TCP_NODELAY");
      exit(-1);
    }
  }
  SessionMessage sessionMessage = {};
  if (ReadN(acceptsock, &sessionMessage, sizeof(sessionMessage)) !=
      sizeof(sessionMessage)) {
    perror("Cannot read session message");
    exit(-1);
  }

  sessionMessage.number = ntohl(sessionMessage.number);
  sessionMessage.length = ntohl(sessionMessage.length);

  PayloadMessage* payloadMessage = reinterpret_cast<PayloadMessage*>(
      malloc(sizeof(PayloadMessage) + sessionMessage.length));
  Base::ScopeGuard guard1([=]() { free(payloadMessage); });

  for (uint32_t i = 0; i < sessionMessage.number; ++i) {
    payloadMessage->length = 0;
    if (ReadN(acceptsock, &payloadMessage->length,
              sizeof(payloadMessage->length)) !=
        sizeof(payloadMessage->length)) {
      perror("Cannot read payload message");
      exit(-1);
    }

    payloadMessage->length = ntohl(payloadMessage->length);

    if (ReadN(acceptsock, payloadMessage->data, payloadMessage->length) !=
        static_cast<int>(payloadMessage->length)) {
      perror("Cannot read payload message");
      exit(-1);
    }

    uint32_t ack = htonl(payloadMessage->length);

    if (WriteN(acceptsock, &ack, sizeof(ack)) !=
        static_cast<int>(sizeof(ack))) {
      perror("Cannot write ack");
      exit(-1);
    }
  }
}

void Sender() {
  auto host = Base::Config::GetGloablDefaultConfig().GetOrDefault<std::string>(
      "/ttcp/server-host", std::string("localhost"));
  auto port = Base::Config::GetGloablDefaultConfig().GetOrDefault<uint16_t>(
      "/ttcp/server-port", 6666);
  auto tcpNodelay = Base::Config::GetGloablDefaultConfig().GetOrDefault<bool>(
      "/ttcp/tcpNoDelay", false);

  int sendsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  Base::ScopeGuard guard([=]() {
    if (sendsock >= 0) close(sendsock);
  });

  if (sendsock < 0) {
    perror("Cannot create sending socket");
    exit(-1);
  }

  auto addr = Net::IpAddress::Resolve(host, std::to_string(port).data());
  if (!addr) {
    perror("Cannot resolve address");
    exit(-1);
  }
  if (connect(sendsock, addr->GetSockaddr(), sizeof(struct sockaddr_in6)) < 0) {
    perror("Cannot connect to the server");
    exit(-1);
  }
  if (tcpNodelay) {
    int flag = 1;
    if (setsockopt(sendsock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) <
        0) {
      perror("Cannot set TCP_NODELAY");
      exit(-1);
    }
  }

  auto number = Base::Config::GetGloablDefaultConfig().GetOrDefault<uint32_t>(
      "/ttcp/message-number", 4096);
  auto length = Base::Config::GetGloablDefaultConfig().GetOrDefault<uint32_t>(
      "/ttcp/message-length", 4096);

  Base::INFO(
      "Start sending. host : {}, port : {}, tcpNoDelay : {}, number : {}, "
      "length : {}",
      host, port, tcpNodelay, number, length);

  auto start = Base::Time::Now();
  SessionMessage sessionMessage = {};
  sessionMessage.number = htonl(number);
  sessionMessage.length = htonl(length);

  if (WriteN(sendsock, &sessionMessage, sizeof(sessionMessage)) !=
      static_cast<int>(sizeof(sessionMessage))) {
    perror("Cannot write session message");
    exit(-1);
  }

  PayloadMessage* payloadMessage = reinterpret_cast<PayloadMessage*>(
      malloc(sizeof(PayloadMessage) + length));
  Base::ScopeGuard guard1([=]() { free(payloadMessage); });

  for (uint i = 0; i < length; ++i) {
    payloadMessage->data[i] = "0123456789ABCDEF"[i % 16];
  }

  payloadMessage->length = htonl(length);

  double mbs = 1.0 * length * number / 1024.0 / 1024.0;
  // fmt 保留三位小数输出
  Base::INFO("Sending {:.3f} MB data", mbs);

  for (uint32_t i = 0; i < number; ++i) {
    if (WriteN(sendsock, payloadMessage, sizeof(PayloadMessage) + length) !=
        static_cast<int>(sizeof(PayloadMessage) + length)) {
      perror("Cannot write payload message");
      exit(-1);
    }

    uint32_t ack = 0;
    if (ReadN(sendsock, &ack, sizeof(ack)) != sizeof(ack)) {
      perror("Cannot read ack");
      exit(-1);
    }
    ack = ntohl(ack);
    assert(ack == length);
  }

  auto elapsed = Base::Time::Now() - start;
  auto milliseconds =
      std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
  auto floatSec = static_cast<double>(milliseconds) / 1000;

  Base::INFO("Send {} messages in {:.3f} seconds", number, floatSec);
  Base::INFO("Speed: {:.3f} MB/s", mbs / floatSec);
}

int main(int argc, char** argv) {
  if (argc < 2) {
    Base::INFO("Usage: {} <send|recv>\n", argv[0]);
    return -1;
  }
  if (strcmp(argv[1], "send") == 0) {
    Sender();
  } else if (strcmp(argv[1], "recv") == 0) {
    Receiver();
  } else {
    Base::INFO("Usage: {} <send|recv>\n", argv[0]);
    return -1;
  }
  return 0;
}