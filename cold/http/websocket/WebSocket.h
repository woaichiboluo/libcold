#ifndef COLD_HTTP_WEBSOCKET_WEBSOCKET
#define COLD_HTTP_WEBSOCKET_WEBSOCKET

#include "../../coroutines/Channel.h"
#include "../../net/TcpSocket.h"
#include "WebSocketParser.h"

namespace Cold::Http {

class WebSocket : public std::enable_shared_from_this<WebSocket> {
 public:
  friend class WebSocketServer;

  WebSocket(std::string requestUrl, TcpSocket socket)
      : requestUrl_(std::move(requestUrl)), socket_(std::move(socket)) {
    onClose_ = [](std::shared_ptr<WebSocket> self) {};

    onMessage_ = [](std::shared_ptr<WebSocket>, std::string_view data) {
      INFO("WebSocket message: {}", data);
    };
  }

  ~WebSocket() = default;

  WebSocket(const WebSocket&) = delete;
  WebSocket& operator=(const WebSocket&) = delete;

  void Send(std::string_view data, bool binary = false) {
    Send(std::string(data), binary);
  }

  void Send(std::string data, bool binary = false) {
    if (!socket_.IsConnected()) return;
    socket_.GetIoContext().CoSpawn(
        [](std::string d, bool b, std::shared_ptr<WebSocket> self) -> Task<> {
          WebSocketFrame frame;
          frame.fin = 1;
          frame.mask = 0;
          frame.opcode = b ? 0x2 : 0x1;
          frame.payloadView = d;
          std::string buf;
          WebSocketParser::MakeFrameToBuffer(frame, buf);
          co_await self->writeMessages_.Write(std::move(buf));
        }(std::move(data), binary, shared_from_this()));
  }

  void Send(const char* data, size_t len, bool binary = false) {
    Send(std::string(data, len), binary);
  }

  void SendPingPong(bool ping) {
    std::string buf;
    WebSocketFrame frame;
    frame.fin = 1;
    frame.mask = 0;
    frame.opcode = ping ? 0x9 : 0xa;
    WebSocketParser::MakeFrameToBuffer(frame, buf);
    socket_.GetIoContext().CoSpawn(
        [](std::string b, std::shared_ptr<WebSocket> self) -> Task<> {
          co_await self->writeMessages_.Write(std::move(b));
        }(std::move(buf), shared_from_this()));
  }

  void SetOnMessage(
      std::function<void(std::shared_ptr<WebSocket>, std::string_view)>
          onMessage) {
    onMessage_ = std::move(onMessage);
  }

  void SetOnClose(std::function<void(std::shared_ptr<WebSocket>)> onClose) {
    onClose_ = std::move(onClose);
  }

  void Close() {
    auto self = shared_from_this();
    socket_.GetIoContext().CoSpawn(Stop(self));
  }

  IoContext& GetIoContext() const { return socket_.GetIoContext(); }

  std::string_view GetRequestUrl() const { return requestUrl_; }

 private:
  Task<> DoRead(std::shared_ptr<WebSocket> self) {
    WebSocketParser parser;
    char buf[65536];
    while (socket_.IsConnected() && !writeMessages_.IsClosed()) {
      auto n = co_await socket_.Read(buf, sizeof buf);
      if (n <= 0) {
        co_await Stop(self);
        co_return;
      }
      // parse frame
      if (!parser.Parse(buf, static_cast<size_t>(n))) {
        co_await Stop(self);
        co_return;
      }
      if (!parser.HasFrame()) continue;
      auto frame = parser.TakeFrame();
      assert(frame.fin == 1);
      switch (frame.opcode) {
        case 0x1:  // text
        case 0x2:  // binary
          if (onMessage_) onMessage_(self, frame.payload);
          break;
        case 0x8:
          co_await Stop(self);
          break;
        case 0x9:  // ping
          SendPingPong(true);
          break;
        case 0xa:  // pong
          SendPingPong(false);
          break;
        default:
          co_await Stop(self);
          break;
      }
    }
  }

  Task<> DoWrite(std::shared_ptr<WebSocket> self) {
    while (!writeMessages_.IsClosed()) {
      auto data = co_await writeMessages_.Read();
      if (data.empty()) {
        co_return;
      }
      auto n = co_await socket_.WriteN(data.data(), data.size());
      if (n != static_cast<ssize_t>(data.size())) {
        break;
      }
    }
    co_await Stop(self);
  }

  Task<> Stop(std::shared_ptr<WebSocket> self) {
    if (stop_) co_return;
    if (onClose_) onClose_(self);
    co_await writeMessages_.Close();
    socket_.Close();
    stop_ = true;
  }

  std::string requestUrl_;
  Channel<std::string> writeMessages_;
  TcpSocket socket_;

  std::atomic<bool> stop_ = false;

  std::function<void(std::shared_ptr<WebSocket>, std::string_view)> onMessage_;
  std::function<void(std::shared_ptr<WebSocket>)> onClose_;
};

}  // namespace Cold::Http

#endif /* COLD_HTTP_WEBSOCKET_WEBSOCKET */
