#include <deque>
#include <memory>
#include <set>
#include <vector>

#include "cold/coro/AsyncEvent.h"
#include "cold/coro/IoService.h"
#include "cold/coro/IoServicePool.h"
#include "cold/net/Acceptor.h"
#include "cold/net/TcpSocket.h"
#include "cold/thread/Lock.h"
#include "examples/asio/chatroom/ChatMessage.h"

using namespace Cold;

class ChatRoom;
class ChatConnection;
using ConnectionPtr = std::shared_ptr<ChatConnection>;

class ChatConnection : public std::enable_shared_from_this<ChatConnection> {
 public:
  ChatConnection(ChatRoom& room, Net::TcpSocket socket);

  void Deliver(const ChatMessage& message) {
    {
      Base::LockGuard guard(mutex_);
      temp_.insert(temp_.end(), message.Data(),
                   message.Data() + message.Length());
    }
    event_.Set();
  }

  void DoChat() {
    socket_.GetIoService().CoSpawn(DoRead(shared_from_this()));
    socket_.GetIoService().CoSpawn(DoWrite(shared_from_this()));
  }

 private:
  [[nodiscard]] Base::Task<> DoRead(ConnectionPtr self);
  [[nodiscard]] Base::Task<> DoWrite(ConnectionPtr self);

  ChatRoom& room_;
  Base::Mutex mutex_;
  Base::AsyncEvent event_;
  std::vector<char> temp_;
  std::vector<char> buffer_;
  Net::TcpSocket socket_;
};

class ChatRoom {
 public:
  ChatRoom(Base::IoService& service) : service_(service) {}
  ~ChatRoom() = default;

  void Join(ConnectionPtr ptr) {
    service_.CoSpawn([](auto* self, auto conn) -> Base::Task<> {
      self->connections_.insert(conn);
      for (const auto& message : self->hisotryMessages_) {
        conn->Deliver(message);
      }
      co_return;
    }(this, std::move(ptr)));
  }

  void Deliver(const ChatMessage& message) {
    service_.CoSpawn([](auto* self, auto msg) -> Base::Task<> {
      self->hisotryMessages_.push_back(msg);
      if (self->hisotryMessages_.size() > kMaxHistoryMessagesSize)
        self->hisotryMessages_.pop_front();
      for (auto& conn : self->connections_) {
        conn->Deliver(msg);
      }
      co_return;
    }(this, message));
  }

  void Leave(ConnectionPtr ptr) {
    service_.CoSpawn([](auto* self, auto conn) -> Base::Task<> {
      self->connections_.erase(conn);
      co_return;
    }(this, std::move(ptr)));
  }

 private:
  static constexpr size_t kMaxHistoryMessagesSize = 100;
  Base::IoService& service_;
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
      room_.Deliver(message);
      continue;
    }
    ret = co_await socket_.ReadN(message.Body(), message.BodyLength());
    if (ret <= 0) {
      socket_.Close();
      break;
    }
    room_.Deliver(message);
  }
  room_.Leave(self);
  event_.Set();
}

Base::Task<> ChatConnection::DoWrite(ConnectionPtr self) {
  while (socket_.IsConnected()) {
    {
      Base::LockGuard guard(mutex_);
      buffer_.swap(temp_);
    }
    auto ret = co_await socket_.WriteN(buffer_.data(), buffer_.size());
    if (ret < 0) {
      socket_.Close();
      co_return;
    }
    buffer_.clear();
    co_await event_;
  }
}

class ChatServer {
 public:
  ChatServer(size_t poolSize, const Net::IpAddress& addr)
      : pool_(poolSize),
        acceptor_(pool_.GetMainIoService(), addr, true),
        room_(pool_.GetMainIoService()) {}
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
        auto conn = std::make_shared<ChatConnection>(room_, std::move(sock));
        room_.Join(conn);
        conn->DoChat();
      }
    }
  }

 private:
  Base::IoServicePool pool_;
  Net::Acceptor acceptor_;
  ChatRoom room_;
};

int main() {
  // Base::LogManager::Instance().GetMainLogger()->SetLevel(Base::LogLevel::TRACE);
  Net::IpAddress addr(8888);
  ChatServer server(4, addr);
  Base::INFO("ChatServer run at port : {}", addr.GetPort());
  server.Start();
}