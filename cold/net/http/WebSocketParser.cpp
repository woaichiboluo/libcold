#include "cold/net/http/WebSocketParser.h"

#include <cassert>

#include "cold/net/Endian.h"

using namespace Cold;

bool Net::Http::WebSocketParser::Parse(const char* data, size_t len) {
  buffer_.insert(buffer_.end(), data, data + len);
  auto state = DoParse();
  while (state == kSuccess) {
    state = DoParse();
  }
  return state != kError;
}

Net::Http::WebSocketParser::ParseState Net::Http::WebSocketParser::DoParse() {
  if (newFrame_) {
    frames_.push_back({});
  }
  newFrame_ = false;
  auto& frame = frames_.back();
  size_t expectLen = 2;
  // 1 byte FIN RSV1 RSV2 RSV3 OPCODE
  // 1 + 2 + 8 = 11 byte byte MASK (PAYLOAD LENGTH 7 bit or 2 byte or 8 byte)
  // 4 byte MASKING KEY
  // rest: PAYLOAD DATA == PAYLOAD LENGTH
  if (buffer_.size() < expectLen) return kNeedMore;
  // 0     1    2    3    4 5 6 7   0      1  2   3  4  5  6  7  8
  // FIN RSV1 RSV2 RSV3   opcode   mask        payload length
  // fin 1000 0000
  frame.fin = (buffer_[0] & 0x80);
  // rsv for extension ignore it
  //   int rsv1 = (buffer_[0] & 0x40);
  //   int rsv2 = (buffer_[0] & 0x20);
  //   int rsv3 = (buffer_[0] & 0x10);
  frame.opcode = buffer_[0] & 0x0f;
  // reserved opcode
  if ((3 <= frame.opcode && frame.opcode <= 7) || (frame.opcode == 0xb))
    return kError;
  frame.mask = (buffer_[1] & 0x80);
  expectLen += frame.mask ? 4 : 0;
  frame.payloadLen = buffer_[1] & 0x7f;
  uint8_t more = 0;
  if (frame.payloadLen == 126)
    more = 2;
  else if (frame.payloadLen == 127)
    more = 8;
  expectLen += more;
  if (buffer_.size() < expectLen) return kNeedMore;
  char* ptr = nullptr;
  uint16_t payloadLen16 = 0;
  uint64_t payloadLen64 = 0;
  if (more) {
    ptr = more == 2 ? reinterpret_cast<char*>(&payloadLen16)
                    : reinterpret_cast<char*>(&payloadLen64);
    for (size_t i = 0; i < more; ++i) {
      ptr[i] = buffer_[2 + i];
    }
    frame.payloadLen = more == 2 ? Host16ToNetwork16(payloadLen16)
                                 : Host64ToNetwork64(payloadLen64);
  }
  expectLen += frame.payloadLen;
  if (buffer_.size() < expectLen) return kNeedMore;
  for (size_t i = 0; i < 4 && frame.mask; ++i) {
    frame.maskingKey[i] = buffer_[2 + more + i];
  }
  std::string_view payload(buffer_.data() + 2 + more + (frame.mask ? 4 : 0),
                           frame.payloadLen);
  frame.payload.reserve(frame.payloadLen);
  if (frame.mask) {
    for (size_t i = 0; i < frame.payloadLen; ++i) {
      frame.payload.push_back(
          static_cast<char>(payload[i] ^ frame.maskingKey[i % 4]));
    }
  } else {
    frame.payload.append(payload);
  }
  buffer_.erase(buffer_.begin(),
                buffer_.begin() + static_cast<long>(expectLen));
  newFrame_ = true;
  if (frame.fin == 1) {
    // merge all frames to last one
    if (frames_.size() > 1) {
      auto& beginFrame = frames_.front();
      for (size_t i = 1; i < frames_.size(); ++i) {
        assert(frames_[i].opcode == 0);
        beginFrame.payload.append(frames_[i].payload);
      }
      beginFrame.payloadLen = beginFrame.payload.size();
      beginFrame.fin = 1;
      frames_.erase(frames_.begin() + 1, frames_.end());
    }
    completeFrames_.push(std::move(frames_.front()));
    frames_.pop_back();
  }
  return kSuccess;
}

void Net::Http::WebSocketParser::MakeFrameToBuffer(
    WebSocketFrame& frame, std::vector<char>& writeBuffer) {
  assert(frame.fin == 1);
  writeBuffer.push_back(static_cast<char>(0x80 | frame.opcode));
  uint8_t maskAndLen = frame.mask ? 0x80 : 0;
  auto size = frame.payloadView.size();
  if (size < 126) {
    maskAndLen |= static_cast<uint8_t>(size);
    writeBuffer.push_back(static_cast<char>(maskAndLen));
  } else if (size < 65536) {
    maskAndLen |= 126;
    writeBuffer.push_back(static_cast<char>(maskAndLen));
    uint16_t cur = Host16ToNetwork16(static_cast<uint16_t>(size));
    auto ptr = reinterpret_cast<const char*>(&cur);
    writeBuffer.insert(writeBuffer.end(), ptr, ptr + 2);
  } else {
    maskAndLen |= 127;
    auto cur = Host64ToNetwork64(static_cast<uint64_t>(size));
    auto ptr = reinterpret_cast<const char*>(&cur);
    writeBuffer.push_back(static_cast<char>(maskAndLen));
    writeBuffer.insert(writeBuffer.end(), ptr, ptr + 8);
  }
  if (frame.mask) {
    uint8_t maskingKey[4];
    for (size_t i = 0; i < 4; ++i) {
      maskingKey[i] = static_cast<uint8_t>(rand() & 0xff);
      writeBuffer.push_back(static_cast<char>(maskingKey[i]));
    }
    for (size_t i = 0; i < size; ++i) {
      writeBuffer.push_back(
          static_cast<char>(frame.payloadView[i] ^ maskingKey[i % 4]));
    }
  } else {
    writeBuffer.insert(writeBuffer.end(), frame.payloadView.begin(),
                       frame.payloadView.end());
  }
}