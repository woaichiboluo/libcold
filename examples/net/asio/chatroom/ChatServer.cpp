#include <set>

#include "Message.h"
#include "cold/Cold.h"

using namespace Cold;

class ChatRoom;

class ChatConnection : public std::enable_shared_from_this<ChatConnection> {
 public:
  ChatConnection(TcpSocket socket, ChatRoom& room)
      : room_(room), socket_(std::move(socket)) {}

  Task<> Deliver(const ChatMessage& message) {
    co_await channel_.Write(message);
  }

  void DoChat() {
    auto self = shared_from_this();
    socket_.GetIoContext().CoSpawn(Reader(self));
    socket_.GetIoContext().CoSpawn(Writer(self));
  }

 private:
  Task<> Reader(std::shared_ptr<ChatConnection> self);
  Task<> Writer(std::shared_ptr<ChatConnection> self);
  Task<> Stop();

  Channel<ChatMessage> channel_;
  ChatRoom& room_;
  TcpSocket socket_;
};

class ChatRoom {
 public:
  constexpr static size_t kMaxHistoryMessage = 100;

  ChatRoom() = default;
  ~ChatRoom() = default;

  Task<> Join(std::shared_ptr<ChatConnection> conn) {
    auto lock = co_await mutex_.ScopedLock();
    connections_.insert(conn);
    for (const auto& msg : historyMessage_) {
      co_await conn->Deliver(msg);
    }
  }

  Task<> Deliver(const ChatMessage& msg) {
    auto lock = co_await mutex_.ScopedLock();
    historyMessage_.push_back(msg);
    if (historyMessage_.size() > kMaxHistoryMessage) {
      historyMessage_.pop_front();
    }
    for (const auto& conn : connections_) {
      co_await conn->Deliver(msg);
    }
  }

  Task<> Leave(std::shared_ptr<ChatConnection> conn) {
    auto lock = co_await mutex_.ScopedLock();
    connections_.erase(conn);
  }

 private:
  AsyncMutex mutex_;
  std::set<std::shared_ptr<ChatConnection>> connections_;
  std::deque<ChatMessage> historyMessage_;
};

Task<> ChatConnection::Reader(std::shared_ptr<ChatConnection> self) {
  ChatMessage message;
  while (socket_.CanReading()) {
    auto n = co_await socket_.ReadN(message.GetData(), message.kHeaderLength);
    if (n != message.kHeaderLength) {
      co_await self->Stop();
      co_return;
    }
    message.Decode();
    auto bodyLength = message.GetBodyLength();
    n = co_await socket_.ReadN(message.GetBody(), bodyLength);
    if (n != bodyLength) {
      co_await self->Stop();
      co_return;
    }
    co_await room_.Deliver(message);
  }
}

Task<> ChatConnection::Writer(std::shared_ptr<ChatConnection> self) {
  while (socket_.CanWriting()) {
    auto message = co_await channel_.Read();
    if (channel_.IsClosed()) co_return;
    auto n = co_await socket_.WriteN(message.GetData(), message.GetLength());
    if (n != message.GetLength()) {
      co_await self->Stop();
      co_return;
    }
  }
}

Task<> ChatConnection::Stop() {
  co_await room_.Leave(shared_from_this());
  co_await channel_.Close();
  socket_.Close();
}

class ChatServer : public TcpServer {
 public:
  ChatServer(const IpAddress& addr, size_t poolSize = 0)
      : TcpServer(addr, poolSize) {}
  ~ChatServer() override = default;

 private:
  Task<> OnNewConnection(TcpSocket socket) override {
    auto conn = std::make_shared<ChatConnection>(std::move(socket), room_);
    co_await room_.Join(conn);
    conn->DoChat();
  }

  ChatRoom room_;
};

int main() {
  ChatServer server(IpAddress(8888), 4);
  INFO("Chat server started at 8888");
  server.Start();
}