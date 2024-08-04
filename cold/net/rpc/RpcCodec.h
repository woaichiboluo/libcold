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
    readBuf_.insert(readBuf_.end(), buf, buf + len);
    uint64_t messageSize = 0;
    if (readBuf_.size() < sizeof(messageSize)) return {false, {}};
    memcpy(&messageSize, readBuf_.data(), sizeof(messageSize));
    messageSize = Net::Network64ToHost64(messageSize);
    if (readBuf_.size() < messageSize + sizeof(messageSize)) return {false, {}};
    return {true, std::string_view{readBuf_.data() + sizeof(messageSize),
                                   messageSize}};
  }

  void TakeMessage(std::string_view message) {
    assert(readBuf_.size() >= message.size());
    auto size = message.size() + sizeof(uint64_t);
    readBuf_.erase(readBuf_.begin(),
                   readBuf_.begin() + static_cast<long>(size));
  }

  bool WriteMessageToBuffer(const google::protobuf::Message& message) {
    writeBuf_.clear();
    uint64_t size = message.ByteSizeLong();
    writeBuf_.resize(size + sizeof(size));
    size = Net::Host64ToNetwork64(size);
    const char* begin = reinterpret_cast<const char*>(&size);
    memcpy(writeBuf_.data(), begin, sizeof(size));
    return message.SerializePartialToArray(
        writeBuf_.data() + sizeof(size),
        static_cast<int>(message.ByteSizeLong()));
  }

  const std::vector<char>& GetReadBuffer() const { return readBuf_; }
  const std::vector<char>& GetWriteBuffer() const { return writeBuf_; }
  std::vector<char>& GetMutableReadBuffer() { return readBuf_; }
  std::vector<char>& GetMutableWriteBuffer() { return writeBuf_; }

 private:
  std::vector<char> readBuf_;
  std::vector<char> writeBuf_;
};

}  // namespace Cold::Net::Rpc

#endif /* NET_RPC_RPCCODEC */
