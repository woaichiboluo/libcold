#ifndef ASIO_CHATROOM_CHATMESSAGE
#define ASIO_CHATROOM_CHATMESSAGE

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "cold/net/Endian.h"

class ChatMessage {
 public:
  static constexpr std::size_t kHeaderLength = sizeof(uint32_t);
  static constexpr std::size_t kMaxBodyLength = 512;

  ChatMessage() : bodyLength_(0) {}

  const char* Data() const { return data_; }

  char* Data() { return data_; }

  std::size_t Length() const { return kHeaderLength + bodyLength_; }

  const char* Body() const { return data_ + kHeaderLength; }

  char* Body() { return data_ + kHeaderLength; }

  std::size_t BodyLength() const { return bodyLength_; }

  void BodyLength(std::size_t newLength) {
    bodyLength_ = newLength;
    if (bodyLength_ > kMaxBodyLength) bodyLength_ = kMaxBodyLength;
  }

  bool DecodeHeader() {
    uint32_t value = 0;
    std::memcpy(&value, data_, kHeaderLength);
    bodyLength_ = Cold::Net::Network32ToHost32(value);
    if (bodyLength_ > kMaxBodyLength) {
      bodyLength_ = 0;
      return false;
    }
    return true;
  }

  void EncodeHeader() {
    auto value =
        Cold::Net::Host32ToNetwork32(static_cast<uint32_t>(bodyLength_));
    std::memcpy(data_, &value, kHeaderLength);
  }

 private:
  char data_[kHeaderLength + kMaxBodyLength];
  std::size_t bodyLength_;
};

#endif /* ASIO_CHATROOM_CHATMESSAGE */
