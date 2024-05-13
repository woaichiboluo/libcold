#include <cstdlib>
#include <deque>
#include <iostream>

#include "cold/coro/IoService.h"
#include "cold/net/IpAddress.h"
#include "cold/net/TcpSocket.h"
#include "cold/thread/Thread.h"
#include "examples/asio/chatroom/ChatMessage.h"

using namespace Cold;

typedef std::deque<ChatMessage> chat_message_queue;

class ChatClient {
 public:
  ChatClient(Base::IoService& service, const Net::IpAddress& addr)
      : socket_(service) {
    socket_.GetIoService().CoSpawn(DoConnect(addr));
  }

  Base::Task<> DoConnect(Net::IpAddress addr) {
    auto ret = co_await socket_.Connect(addr);
    if (ret < 0) {
      Base::INFO("Connect error. errno = {}, reason = {}", errno,
                 Base::ThisThread::ErrorMsg());
      co_return;
    }
    Base::INFO("Connect Success.");
    socket_.GetIoService().CoSpawn(DoRead());
  }

  void Write(const ChatMessage& message) {
    if (socket_.IsConnected()) socket_.GetIoService().CoSpawn(DoWrite(message));
  }

 private:
  Base::Task<> DoWrite(ChatMessage message) {
    if (!socket_.IsConnected()) co_return;
    temp_.push_back(message);
    if (isWriting_) co_return;
    isWriting_ = true;
    messageBuf_.swap(temp_);
    for (const auto& msg : messageBuf_) {
      auto ret = co_await socket_.WriteN(msg.Data(), msg.Length());
      if (ret < 0) {
        socket_.Close();
        co_return;
      }
    }
    messageBuf_.clear();
    isWriting_ = false;
  }

  Base::Task<> DoRead() {
    while (socket_.IsConnected()) {
      ChatMessage message;
      auto ret = co_await socket_.ReadN(message.Data(), message.kHeaderLength);
      if (ret <= 0 || !message.DecodeHeader()) {
        socket_.Close();
        break;
      }
      if (!message.DecodeHeader()) {
        socket_.Close();
        break;
      }
      if (message.BodyLength() == 0) {
        fmt::println("");
        continue;
      }
      ret = co_await socket_.ReadN(message.Body(), message.BodyLength());
      if (ret <= 0) {
        socket_.Close();
        break;
      }
      fmt::println("{}",
                   std::string_view{message.Body(), message.BodyLength()});
    }
  }

  std::deque<ChatMessage> temp_;
  std::deque<ChatMessage> messageBuf_;
  Net::TcpSocket socket_;
  bool isWriting_ = false;
};

int main() {
  Base::IoService service;
  ChatClient c(service, Net::IpAddress(8888));

  Base::Thread thread([&service]() { service.Start(); });
  thread.Start();

  char line[ChatMessage::kMaxBodyLength + 1];
  while (std::cin.getline(line, ChatMessage::kMaxBodyLength + 1)) {
    ChatMessage msg;
    msg.BodyLength(std::strlen(line));
    std::memcpy(msg.Body(), line, msg.BodyLength());
    msg.EncodeHeader();
    c.Write(msg);
  }
  thread.Join();
  return 0;
}