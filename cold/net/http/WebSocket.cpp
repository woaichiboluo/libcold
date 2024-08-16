#include "cold/net/http/WebSocket.h"

#include "cold/net/http/WebSocketParser.h"

using namespace Cold;

Base::Task<> Net::Http::WebSocket::DoRead() {
  if (!socket_.IsConnected()) co_return;
  WebSocketParser parser;
  char buf[65536];
  while (true) {
    auto n = co_await socket_.Read(buf, sizeof buf);
    if (n <= 0) {
      socket_.Close();
      co_return;
    }
    // parse frame
    if (parser.Parse(buf, static_cast<size_t>(n)) == WebSocketParser::kError) {
      OnError();
      co_return;
    }
    auto frame = parser.TakeFrame();
    Base::INFO("fin:{}", frame.fin);
    Base::INFO("opcode:{}", frame.opcode);
    Base::INFO("mask:{}", frame.mask);
    Base::INFO("payloadLen:{}", frame.payloadLen);
    Base::INFO("WebSocket recv: {}",
               std::string_view(frame.payload.data(), frame.payload.size()));
    assert(frame.fin == 1);
    switch (frame.opcode) {
      case 0x1:  // text
      case 0x2:  // binary
        onRecv_(shared_from_this(), frame.payload.data(), frame.payload.size());
        break;
      case 0x8:
        Close();
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
  socket_.GetIoService().CoSpawn(
      [&](std::string buf, WebSocketPtr self) -> Base::Task<> {
        WebSocketFrame frame;
        frame.fin = 1;
        frame.mask = 0;
        frame.opcode = binary ? 0x2 : 0x1;
        frame.payloadView = buf;
        WebSocketParser::MakeFrameToBuffer(frame, writeTempBuffer_);
        if (isWriting_) co_return;
        co_await DoWrite();
      }(std::string(data, len), shared_from_this()));
}

void Net::Http::WebSocket::SendPong() {
  if (!socket_.IsConnected()) return;
  socket_.GetIoService().CoSpawn([&](WebSocketPtr self) -> Base::Task<> {
    WebSocketFrame frame;
    frame.fin = 1;
    frame.mask = 0;
    frame.opcode = 0xa;
    WebSocketParser::MakeFrameToBuffer(frame, writeTempBuffer_);
    if (isWriting_) co_return;
    co_await DoWrite();
  }(shared_from_this()));
}

Base::Task<> Net::Http::WebSocket::DoWrite() {
  isWriting_ = true;
  while (socket_.IsConnected()) {
    writeBuffer_.clear();
    writeBuffer_.swap(writeTempBuffer_);
    if (writeTempBuffer_.empty()) break;
    if (co_await socket_.WriteN(writeBuffer_.data(), writeBuffer_.size()) !=
        static_cast<ssize_t>(writeBuffer_.size())) {
      OnError();
      break;
    }
  }
  isWriting_ = false;
}

void Net::Http::WebSocket::Close() {
  // TODO Send close frame.
  onClose_(shared_from_this());
}

void Net::Http::WebSocket::OnError() {
  onClose_(shared_from_this());
  socket_.Close();
}