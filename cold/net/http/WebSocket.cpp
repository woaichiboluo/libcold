#include "cold/net/http/WebSocket.h"

#include "cold/net/http/WebSocketParser.h"

using namespace Cold;

Base::Task<> Net::Http::WebSocket::DoRead() {
  WebSocketParser parser;
  char buf[65536];
  lastPingTime_ = Base::Time::Now();
  static int timeout = Base::Config::GetGloablDefaultConfig().GetOrDefault(
      "/websocket/pingpong-timeout-second", 10);
  socket_.GetIoService().CoSpawn(SendPing(shared_from_this()));
  while (socket_.IsConnected()) {
    if (Base::Time::Now() - lastPingTime_ > std::chrono::seconds(timeout)) {
      OnError();
      co_return;
    }
    auto n = co_await socket_.Read(buf, sizeof buf);
    if (n <= 0) {
      socket_.Close();
      co_return;
    }
    // parse frame
    if (!parser.Parse(buf, static_cast<size_t>(n))) {
      OnError();
      co_return;
    }
    if (!parser.HasFrame()) continue;
    auto frame = parser.TakeFrame();
    assert(frame.fin == 1);
    switch (frame.opcode) {
      case 0x1:  // text
      case 0x2:  // binary
        onRecv_(shared_from_this(), frame.payload.data(), frame.payload.size());
        break;
      case 0x8:
        onClose_(shared_from_this());
        socket_.Close();
        break;
      case 0x9:  // ping
        SendPong();
        break;
      case 0xa:  // pong
        break;
      default:
        OnError();
        break;
    }
  }
}

void Net::Http::WebSocket::Send(const char* data, size_t len, bool binary) {
  if (!socket_.IsConnected()) return;
  auto aa = shared_from_this();
  socket_.GetIoService().CoSpawn(
      [](std::string buf, WebSocketPtr self, bool b) -> Base::Task<> {
        WebSocketFrame frame;
        frame.fin = 1;
        frame.mask = 0;
        frame.opcode = b ? 0x2 : 0x1;
        frame.payloadView = buf;
        WebSocketParser::MakeFrameToBuffer(frame, self->writeTempBuffer_);
        if (self->isWriting_) co_return;
        co_await self->DoWrite();
      }(std::string(data, len), shared_from_this(), binary));
}

void Net::Http::WebSocket::SendPong() {
  if (!socket_.IsConnected()) return;
  socket_.GetIoService().CoSpawn([](WebSocketPtr self) -> Base::Task<> {
    WebSocketFrame frame;
    frame.fin = 1;
    frame.mask = 0;
    frame.opcode = 0xa;
    WebSocketParser::MakeFrameToBuffer(frame, self->writeTempBuffer_);
    if (self->isWriting_) co_return;
    co_await self->DoWrite();
  }(shared_from_this()));
}

Base::Task<> Net::Http::WebSocket::DoWrite() {
  isWriting_ = true;
  while (socket_.IsConnected()) {
    writeBuffer_.swap(writeTempBuffer_);
    if (writeBuffer_.empty()) break;
    if (co_await socket_.WriteN(writeBuffer_.data(), writeBuffer_.size()) !=
        static_cast<ssize_t>(writeBuffer_.size())) {
      OnError();
      break;
    }
    writeBuffer_.clear();
  }
  isWriting_ = false;
}

void Net::Http::WebSocket::Close() {
  onClose_(shared_from_this());
  socket_.GetIoService().CoSpawn([](WebSocketPtr self) -> Base::Task<> {
    WebSocketFrame frame;
    frame.fin = 1;
    frame.mask = 0;
    frame.opcode = 0x8;
    WebSocketParser::MakeFrameToBuffer(frame, self->writeTempBuffer_);
    if (self->isWriting_) co_return;
    co_await self->DoWrite();
    self->socket_.Close();
  }(shared_from_this()));
}

void Net::Http::WebSocket::OnError() {
  onClose_(shared_from_this());
  socket_.Close();
}

Base::Task<> Net::Http::WebSocket::SendPing(WebSocketPtr self) {
  Base::Timer timer(self->socket_.GetIoService());
  while (socket_.IsConnected()) {
    int sec = Base::Config::GetGloablDefaultConfig().GetOrDefault(
        "/websocket/ping-second", 10);
    if (sec < 0) sec = 10;
    timer.ExpiresAfter(std::chrono::seconds(sec));
    co_await timer.AsyncWaitable([](WebSocketPtr s) -> Base::Task<> {
      s->lastPingTime_ = Base::Time::Now();
      WebSocketFrame frame;
      frame.fin = 1;
      frame.mask = 0;
      frame.opcode = 0x9;
      WebSocketParser::MakeFrameToBuffer(frame, s->writeTempBuffer_);
      if (s->isWriting_) co_return;
      co_await s->DoWrite();
    }(self));
  }
}