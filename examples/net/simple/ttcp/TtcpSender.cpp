#include "Message.h"
#include "cold/Cold.h"

using namespace Cold;

class TtcpSender {
 public:
  TtcpSender(IoContext& ioContext, uint32_t number, uint32_t length)
      : socket_(ioContext), number_(number), length_(length) {}
  ~TtcpSender() = default;

  TtcpSender(const TtcpSender&) = delete;
  TtcpSender& operator=(const TtcpSender&) = delete;

  Task<> Connect(IpAddress addr) {
    using namespace std::chrono_literals;
    auto [timeout, connected] = co_await Timeout(socket_.Connect(addr), 3s);
    if (timeout || !connected) {
      INFO("Connect to {} failed", addr.GetIpPort());
      socket_.GetIoContext().Stop();
      co_return;
    }
    co_await DoTtcp();
  }

  Task<> DoTtcp() {
    auto startTime = Time::Now();
    SessionMessage sessionMessage;
    double sendMb = 1.0 * length_ * number_ / 1024.0 / 1024.0;
    INFO("number: {}, length: {}", number_, length_);
    INFO("total send : {:.3f} MB", sendMb);
    sessionMessage.number = Endian::Host32ToNetwork32(number_);
    sessionMessage.length = Endian::Host32ToNetwork32(length_);
    if (co_await socket_.WriteN(&sessionMessage, sizeof(sessionMessage)) !=
        sizeof(sessionMessage)) {
      FATAL("Write session message failed.");
      co_return;
    }
    auto payloadMessage = reinterpret_cast<PayLoadMessage*>(
        malloc(sizeof(PayLoadMessage) + length_));
    ScopeGuard guard([=]() { free(payloadMessage); });
    payloadMessage->length = Endian::Host32ToNetwork32(length_);
    for (uint i = 0; i < length_; ++i) {
      payloadMessage->data[i] = "0123456789ABCDEF"[i % 16];
    }
    ssize_t payLoadMessageSize = sizeof(PayLoadMessage) + length_;
    for (uint32_t i = 0; i < number_; ++i) {
      if (co_await socket_.WriteN(payloadMessage,
                                  static_cast<size_t>(payLoadMessageSize)) !=
          payLoadMessageSize) {
        FATAL("Write payload message failed");
        co_return;
      }
      uint32_t ack = 0;
      if (co_await socket_.ReadN(&ack, sizeof(ack)) != sizeof(ack)) {
        FATAL("Read ack failed");
        co_return;
      }
      ack = Endian::Network32ToHost32(ack);
      if (ack != length_) {
        FATAL("Ack is not equal to length");
        co_return;
      }
    }
    auto endTime = Time::Now();
    auto elapsedMilliSec =
        std::chrono::duration_cast<std::chrono::milliseconds>(endTime -
                                                              startTime)
            .count();
    double elapsedSec = static_cast<double>(elapsedMilliSec) / 1000;
    double mibs = sendMb / elapsedSec;
    double mbs = mibs * 8;
    INFO("speed: {:.3f} MiB/s {:.3f} MB/s", mibs, mbs);
    socket_.GetIoContext().Stop();
  }

 private:
  TcpSocket socket_;
  uint32_t number_;
  uint32_t length_;
};

int main(int argc, char** argv) {
  if (argc != 3) {
    INFO("Usage: {} <number> <length>", argv[0]);
    return 1;
  }
  uint32_t number = static_cast<uint32_t>(std::stoi(argv[1]));
  uint32_t length = static_cast<uint32_t>(std::stoi(argv[2]));
  Cold::IoContext ioContext;
  TtcpSender sender(ioContext, number, length);
  ioContext.CoSpawn(sender.Connect(IpAddress(8888)));
  ioContext.Start();
}