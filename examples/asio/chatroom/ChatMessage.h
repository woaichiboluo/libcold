#ifndef ASIO_CHATROOM_CHATMESSAGE
#define ASIO_CHATROOM_CHATMESSAGE

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

class ChatMessage {
 public:
  static constexpr std::size_t kHeaderLength = 4;
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
    char header[kHeaderLength + 1] = "";
    std::strncat(header, data_, kHeaderLength);
    bodyLength_ = static_cast<size_t>(std::atoi(header));
    if (bodyLength_ > kMaxBodyLength) {
      bodyLength_ = 0;
      return false;
    }
    return true;
  }

  void EncodeHeader() {
    char header[kHeaderLength + 1] = "";
    std::sprintf(header, "%4d", static_cast<int>(bodyLength_));
    std::memcpy(data_, header, kHeaderLength);
  }

 private:
  char data_[kHeaderLength + kMaxBodyLength];
  std::size_t bodyLength_;
};

#endif /* ASIO_CHATROOM_CHATMESSAGE */
