#include "Message.h"
#include "cold/Cold.h"

using namespace Cold;

class TtcpReceiver : public TcpServer {
 public:
  TtcpReceiver(const IpAddress& addr) : TcpServer(addr) {}
  ~TtcpReceiver() = default;

  Task<> DoTtcp(TcpSocket socket) {
    SessionMessage message = {};
    if (co_await socket.ReadN(&message, sizeof(message)) != sizeof(message)) {
      FATAL("Cannot read session message.");
    }
    message.number = Endian::Network32ToHost32(message.number);
    message.length = Endian::Network32ToHost32(message.length);

    auto payloadMessage = reinterpret_cast<PayLoadMessage*>(
        malloc(sizeof(PayLoadMessage) + message.length));
    ScopeGuard guard([=]() { free(payloadMessage); });
    for (uint32_t i = 0; i < message.number; ++i) {
      payloadMessage->length = 0;
      if (co_await socket.ReadN(&payloadMessage->length,
                                sizeof(payloadMessage->length)) !=
          sizeof(payloadMessage->length)) {
        FATAL("Cannot read payload message");
      }

      payloadMessage->length =
          Endian::Network32ToHost32(payloadMessage->length);

      if (co_await socket.ReadN(payloadMessage->data, payloadMessage->length) !=
          payloadMessage->length) {
        FATAL("Cannot read payload message");
      }

      uint32_t ack = Endian::Host32ToNetwork32(payloadMessage->length);

      if (co_await socket.WriteN(&ack, sizeof(ack)) !=
          static_cast<int>(sizeof(ack))) {
        FATAL("Cannot write ack");
      }
    }
  }

  Task<> OnNewConnection(TcpSocket socket) override {
    co_await DoTtcp(std::move(socket));
  }
};

int main() {
  TtcpReceiver receiver(IpAddress(8888));
  INFO("TtcpReceiver Run at {}", receiver.GetLocalAddress().GetIpPort());
  receiver.Start();
}