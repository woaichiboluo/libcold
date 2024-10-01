#ifndef EXAMPLES_NET_ASIO_CHATROOM_MESSAGE
#define EXAMPLES_NET_ASIO_CHATROOM_MESSAGE

#include <cassert>

#include "cold/util/IntWriter.h"

class ChatMessage {
 public:
  constexpr static size_t kHeaderLength = 4;
  constexpr static size_t kMaxBodyLength = 512;

  ChatMessage() = default;
  ~ChatMessage() = default;

  void Encode() {
    [[maybe_unused]] auto e = Cold::WriteInt(bodyLength_, data_);
    assert(e == data_ + kHeaderLength);
  }

  void Decode() {
    [[maybe_unused]] auto e = Cold::ReadInt(bodyLength_, data_);
    assert(e == data_ + kHeaderLength);
  }

  void SetBodyLength(uint32_t length) { bodyLength_ = length; }
  uint32_t GetBodyLength() const { return bodyLength_; }

  uint32_t GetLength() const { return kHeaderLength + bodyLength_; }

  const char* GetBody() const { return data_ + kHeaderLength; }
  char* GetBody() { return data_ + kHeaderLength; }

  const char* GetData() const { return data_; }
  char* GetData() { return data_; }

 private:
  uint32_t bodyLength_ = 0;
  char data_[kHeaderLength + kMaxBodyLength];
};

#endif /* EXAMPLES_NET_ASIO_CHATROOM_MESSAGE */
