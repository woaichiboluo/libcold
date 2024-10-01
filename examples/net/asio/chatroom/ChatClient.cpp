#include "Message.h"
#include "cold/Cold.h"

using namespace Cold;

class ChatClient {
 public:
  ChatClient(IoContext& context) : socket_(context) {}
  ~ChatClient() = default;

  ChatClient(const ChatClient&) = delete;
  ChatClient& operator=(const ChatClient&) = delete;

  Task<> ConnectOrStop(IpAddress addr) {
    if (!co_await socket_.Connect(addr)) {
      ERROR("Connect failed. reason: {}", ThisThread::ErrorMsg());
      Stop();
      co_return;
    }
    INFO("connect success");
    socket_.GetIoContext().CoSpawn(Reader());
    co_await Writer();
  }

 private:
  void Stop() {
    socket_.Close();
    socket_.GetIoContext().Stop();
  }

  Task<> Reader() {
    ChatMessage message;
    while (socket_.CanReading()) {
      auto n = co_await socket_.ReadN(message.GetData(), message.kHeaderLength);
      if (n != message.kHeaderLength) {
        Stop();
        co_return;
      }
      message.Decode();
      auto bodyLength = message.GetBodyLength();
      n = co_await socket_.ReadN(message.GetBody(), bodyLength);
      if (n != bodyLength) {
        Stop();
        co_return;
      }
      fmt::println(
          "{}", std::string_view{message.GetBody(), message.GetBodyLength()});
    }
  }

  Task<> Writer() {
    AsyncIo io(socket_.GetIoContext(), STDIN_FILENO);
    ChatMessage message;
    while (socket_.CanWriting()) {
      char buf[ChatMessage::kMaxBodyLength - 1];
      auto n = co_await io.AsyncReadSome(buf, sizeof(buf));
      if (n <= 0) {
        Stop();
        co_return;
      }
      // skip the last '\n'
      message.SetBodyLength(static_cast<uint32_t>(n - 1));
      std::copy(buf, buf + n - 1, message.GetBody());
      message.Encode();
      n = co_await socket_.WriteN(message.GetData(), message.GetLength());
      if (n != message.GetLength()) {
        Stop();
        co_return;
      }
    }
  }

  TcpSocket socket_;
};

int main() {
  IoContext context;
  ChatClient client(context);
  context.CoSpawn(client.ConnectOrStop(IpAddress(8888)));
  context.Start();
}