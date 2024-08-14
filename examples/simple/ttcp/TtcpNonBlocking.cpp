#include "cold/net/SocketOptions.h"
#include "cold/net/TcpClient.h"
#include "cold/net/TcpServer.h"
#include "cold/util/ScopeUtil.h"
#include "examples/simple/ttcp/TtcpCommon.h"

using namespace Cold;

class TtcpReceiver : public Net::TcpServer {
 public:
  TtcpReceiver(const Net::IpAddress& addr) : TcpServer(addr) {}

  Base::Task<> OnConnect(Net::TcpSocket socket) override {
    socket.GetIoService().CoSpawn(DoTtcp(std::move(socket)));
    co_return;
  }

  Base::Task<> DoTtcp(Net::TcpSocket socket) {
    auto tcpNodelay = Base::Config::GetGloablDefaultConfig().GetOrDefault<bool>(
        "/ttcp/tcpNoDelay", false);
    if (tcpNodelay) {
      Net::SocketOptions::TcpNoDelay option(tcpNodelay);
      socket.SetOption(option);
    }
    SessionMessage message = {};

    if (co_await socket.ReadN(reinterpret_cast<char*>(&message),
                              sizeof(message)) != sizeof(message)) {
      perror("Cannot read session message");
      exit(-1);
    }

    message.number = ntohl(message.number);
    message.length = ntohl(message.length);

    PayloadMessage* payloadMessage = reinterpret_cast<PayloadMessage*>(
        malloc(sizeof(PayloadMessage) + message.length));
    Base::ScopeGuard guard([=]() { free(payloadMessage); });

    for (uint32_t i = 0; i < message.number; ++i) {
      payloadMessage->length = 0;
      if (co_await socket.ReadN(
              reinterpret_cast<char*>(&payloadMessage->length),
              sizeof(payloadMessage->length)) !=
          sizeof(payloadMessage->length)) {
        perror("Cannot read payload message");
        exit(-1);
      }

      payloadMessage->length = ntohl(payloadMessage->length);

      if (co_await socket.ReadN(payloadMessage->data, payloadMessage->length) !=
          static_cast<int>(payloadMessage->length)) {
        perror("Cannot read payload message");
        exit(-1);
      }

      uint32_t ack = htonl(payloadMessage->length);

      if (co_await socket.WriteN(reinterpret_cast<char*>(&ack), sizeof(ack)) !=
          static_cast<int>(sizeof(ack))) {
        perror("Cannot write ack");
        exit(-1);
      }
    }
  }
};

class TtcpSender : public Net::TcpClient {
 public:
  TtcpSender(Base::IoService& service, uint32_t number, uint32_t length,
             bool tcpNodelay)
      : TcpClient(service),
        number_(number),
        length_(length),
        tcpNodelay_(tcpNodelay) {}

  Base::Task<> OnConnect() override {
    Base::INFO("Connected to server");
    socket_.GetIoService().CoSpawn(DoTtcp());
    co_return;
  }

  Base::Task<> DoTtcp() {
    auto start = Base::Time::Now();
    double mbs = 1.0 * length_ * number_ / 1024.0 / 1024.0;
    if (tcpNodelay_) {
      Net::SocketOptions::TcpNoDelay option(tcpNodelay_);
      socket_.SetOption(option);
    }
    Base::INFO("total {:.3f} MB", mbs);
    SessionMessage message;
    message.number = htonl(number_);
    message.length = htonl(length_);

    if (co_await socket_.WriteN(reinterpret_cast<char*>(&message),
                                sizeof(message)) != sizeof(message)) {
      perror("Cannot write session message");
      exit(-1);
    }

    PayloadMessage* payloadMessage = reinterpret_cast<PayloadMessage*>(
        malloc(sizeof(PayloadMessage) + length_));
    Base::ScopeGuard guard([=]() { free(payloadMessage); });
    payloadMessage->length = htonl(length_);
    for (uint i = 0; i < length_; ++i) {
      payloadMessage->data[i] = "0123456789ABCDEF"[i % 16];
    }

    for (uint32_t i = 0; i < number_; ++i) {
      if (co_await socket_.WriteN(reinterpret_cast<const char*>(payloadMessage),
                                  sizeof(PayloadMessage) + length_) !=
          static_cast<ssize_t>(sizeof(PayloadMessage) + length_)) {
        perror("Cannot write payload message");
        exit(-1);
      }

      uint32_t ack = 0;
      if (co_await socket_.ReadN(reinterpret_cast<char*>(&ack), sizeof(ack)) !=
          sizeof(ack)) {
        perror("Cannot read ack");
        exit(-1);
      }

      ack = ntohl(ack);
      assert(ack == length_);
    }

    auto elapsed = Base::Time::Now() - start;
    auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    auto floatSec = static_cast<double>(milliseconds) / 1000;

    Base::INFO("Send {} messages in {:.3f} seconds", number_, floatSec);
    Base::INFO("Speed: {:.3f} MB/s", mbs / floatSec);

    socket_.GetIoService().Stop();
  }

 private:
  uint32_t number_;
  uint32_t length_;
  bool tcpNodelay_;
};

int main(int argc, char** argv) {
  // Base::LogManager::Instance().GetMainLogger()->SetLevel(Base::LogLevel::TRACE);
  if (argc < 2) {
    Base::INFO("Usage: {} <send|recv>", argv[0]);
    return -1;
  }
  if (strcmp(argv[1], "send") == 0) {
    auto host =
        Base::Config::GetGloablDefaultConfig().GetOrDefault<std::string>(
            "/ttcp/server-host", std::string("localhost"));
    auto port = Base::Config::GetGloablDefaultConfig().GetOrDefault<uint16_t>(
        "/ttcp/server-port", 6666);
    auto tcpNodelay = Base::Config::GetGloablDefaultConfig().GetOrDefault<bool>(
        "/ttcp/tcpNoDelay", false);
    auto number = Base::Config::GetGloablDefaultConfig().GetOrDefault<uint32_t>(
        "/ttcp/message-number", 4096);
    auto length = Base::Config::GetGloablDefaultConfig().GetOrDefault<uint32_t>(
        "/ttcp/message-length", 4096);
    Base::INFO(
        "Start sending. host : {}, port : {}, tcpNoDelay : {}, number : {}, "
        "length : {}",
        host, port, tcpNodelay, number, length);
    Base::IoService service;
    TtcpSender sender(service, number, length, tcpNodelay);
    auto addr = Net::IpAddress::Resolve(host, std::to_string(port).data());
    if (!addr) {
      Base::ERROR("Cannot resolve address. host: {}, port: {}", host, port);
      return -1;
    }
    service.CoSpawn(sender.Connect(*addr));
    service.Start();
  } else if (strcmp(argv[1], "recv") == 0) {
    auto port = Base::Config::GetGloablDefaultConfig().GetOrDefault<uint16_t>(
        "/ttcp/server-port", 6666);
    Net::IpAddress addr(port);
    TtcpReceiver receiver(addr);
    receiver.Start();
  } else {
    Base::INFO("Usage: {} <send|recv>\n", argv[0]);
    return -1;
  }
  return 0;
}