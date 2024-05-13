#include <deque>
#include <memory>
#include <set>

#include "cold/coro/AsyncMutex.h"
#include "cold/coro/IoServicePool.h"
#include "cold/net/Acceptor.h"
#include "cold/net/TcpSocket.h"
#include "examples/asio/chatroom/ChatMessage.h"

using namespace Cold;

class ChatRoom;
class ChatConnection;
using ConnectionPtr = std::shared_ptr<ChatConnection>;

class ChatConnection : public std::enable_shared_from_this<ChatConnection> {
 public:
  ChatConnection(ChatRoom& room, Net::TcpSocket socket);

  void Deliver(const ChatMessage& message) {
    socket_.GetIoService().CoSpawn(DoWrite(shared_from_this(), message));
  }

  void DoChat() { socket_.GetIoService().CoSpawn(DoRead(shared_from_this())); }

 private:
  [[nodiscard]] Base::Task<> DoRead(ConnectionPtr self);
  [[nodiscard]] Base::Task<> DoWrite(ConnectionPtr self, ChatMessage message);

  ChatRoom& room_;
  bool isWriting_ = false;
  std::deque<ChatMessage> temp_;
  std::deque<ChatMessage> messagesForWrite_;
  Net::TcpSocket socket_;
};

class ChatRoom {
 public:
  ChatRoom() = default;
  ~ChatRoom() = default;

  [[nodiscard]] Base::Task<void> Join(ConnectionPtr ptr) {
    auto guard = co_await mutex_.ScopedLockAsync();
    connections_.insert(ptr);
    for (const auto& message : hisotryMessages_) {
      ptr->Deliver(message);
    }
  }

  [[nodiscard]] Base::Task<void> Deliver(const ChatMessage& message) {
    auto guard = co_await mutex_.ScopedLockAsync();
    hisotryMessages_.push_back(message);
    if (hisotryMessages_.size() > kMaxHistoryMessagesSize)
      hisotryMessages_.pop_front();
    for (auto& conn : connections_) {
      conn->Deliver(message);
    }
  }

  [[nodiscard]] Base::Task<void> Leave(ConnectionPtr ptr) {
    auto guard = co_await mutex_.ScopedLockAsync();
    connections_.erase(ptr);
  }

 private:
  static constexpr size_t kMaxHistoryMessagesSize = 100;
  Base::AsyncMutex mutex_;
  std::deque<ChatMessage> hisotryMessages_;
  std::set<ConnectionPtr> connections_;
};

ChatConnection::ChatConnection(ChatRoom& room, Net::TcpSocket socket)
    : room_(room), socket_(std::move(socket)) {}

Base::Task<> ChatConnection::DoRead(ConnectionPtr self) {
  while (socket_.IsConnected()) {
    ChatMessage message;
    auto ret = co_await socket_.ReadN(message.Data(), message.kHeaderLength);
    if (ret <= 0) {
      socket_.Close();
      break;
    }
    if (!message.DecodeHeader()) {
      socket_.Close();
      break;
    }
    if (message.BodyLength() == 0) {
      co_await room_.Deliver(message);
      continue;
    }
    ret = co_await socket_.ReadN(message.Body(), message.BodyLength());
    if (ret <= 0) {
      socket_.Close();
      break;
    }
    co_await room_.Deliver(message);
  }
  co_await room_.Leave(self);
}

Base::Task<> ChatConnection::DoWrite(ConnectionPtr self, ChatMessage msg) {
  if (!socket_.IsConnected()) co_return;
  temp_.push_back(msg);
  if (isWriting_) co_return;
  isWriting_ = true;
  messagesForWrite_.swap(temp_);
  for (const auto& message : messagesForWrite_) {
    auto ret = co_await socket_.WriteN(message.Data(), message.Length());
    if (ret < 0) {
      socket_.Close();
      co_await room_.Leave(self);
      break;
    }
  }
  messagesForWrite_.clear();
  isWriting_ = false;
}

class ChatServer {
 public:
  ChatServer(size_t poolSize, const Net::IpAddress& addr)
      : pool_(poolSize), acceptor_(pool_.GetMainIoService(), addr, true) {}
  ~ChatServer() = default;

  void Start() {
    acceptor_.Listen();
    acceptor_.GetIoService().CoSpawn(DoAccept());
    pool_.Start();
  }

  [[nodiscard]] Base::Task<> DoAccept() {
    while (true) {
      auto sock = co_await acceptor_.Accept(pool_.GetNextIoService());
      if (sock) {
        auto conn = std::make_shared<ChatConnection>(room, std::move(sock));
        co_await room.Join(conn);
        conn->DoChat();
      }
    }
  }

 private:
  Base::IoServicePool pool_;
  Net::Acceptor acceptor_;
  ChatRoom room;
};

int main() {
  // Base::LogManager::Instance().GetMainLogger()->SetLevel(Base::LogLevel::TRACE);
  Net::IpAddress addr(8888);
  ChatServer server(4, addr);
  Base::INFO("ChatServer run at port : {}", addr.GetPort());
  server.Start();
}