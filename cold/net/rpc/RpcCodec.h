#ifndef NET_RPC_RPCCODEC
#define NET_RPC_RPCCODEC

#include <cassert>
#include <cstdint>
#include <vector>

#include "cold/net/Endian.h"
#include "google/protobuf/message.h"

namespace Cold::Net::Rpc {

// message RpcMessage {
//   bytes service = 1;
//   bytes method = 2;
//   bytes payload = 3;
//   ErrorCode error = 4;
// }

/*
data format:
    | 8 bytes | M = len serilaized message
    | M       | (service,method,payload,error)
*/

class RpcCodec {
 public:
  RpcCodec() = default;

  ~RpcCodec() = default;

  RpcCodec(RpcCodec const&) = delete;
  RpcCodec& operator=(RpcCodec const&) = delete;

  std::pair<bool, std::string_view> ParseMessage(const char* buf, size_t len) {
    buf_.insert(buf_.end(), buf, buf + len);
    uint64_t messageSize = 0;
    if (buf_.size() < sizeof(messageSize)) return {false, {}};
    memcpy(&messageSize, buf_.data(), sizeof(messageSize));
    messageSize = Net::Network64ToHost64(messageSize);
    if (buf_.size() < messageSize + sizeof(messageSize)) return {false, {}};
    return {true,
            std::string_view{buf_.data() + sizeof(messageSize), messageSize}};
  }

  void TakeMessage(std::string_view message) {
    assert(buf_.size() >= message.size());
    auto size = message.size() + sizeof(uint64_t);
    buf_.erase(buf_.begin(), buf_.begin() + static_cast<long>(size));
  }

  bool WriteMessageToBuffer(const google::protobuf::Message& message) {
    buf_.clear();
    uint64_t size = message.ByteSizeLong();
    buf_.reserve(size + sizeof(size));
    const char* begin = reinterpret_cast<const char*>(&size);
    size = Net::Host64ToNetwork64(size);
    buf_.insert(buf_.end(), begin, begin + sizeof(size));
    return message.SerializePartialToArray(buf_.data() + sizeof(size),
                                           static_cast<int>(size));
  }

  const std::vector<char>& GetBuffer() const { return buf_; }
  std::vector<char>& GetMutableBuffer() { return buf_; }

 private:
  std::vector<char> buf_;
};

}  // namespace Cold::Net::Rpc

#endif /* NET_RPC_RPCCODEC */
