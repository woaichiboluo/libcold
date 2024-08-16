#include "cold/net/http/WebSocketParser.h"

#include <cassert>

#include "cold/log/Logger.h"

using namespace Cold;

Net::Http::WebSocketParser::ParseState Net::Http::WebSocketParser::Parse(
    const char* data, size_t len) {
  if (frames_.empty()) {
    frames_.push_back({});
  }
  auto& frame = frames_.back();
  buffer_.insert(buffer_.end(), data, data + len);
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
  if (more) {
    for (size_t i = 0; i < more; ++i) {
      frame.payloadLen =
          (frame.payloadLen << 8) | static_cast<unsigned long>(buffer_[2 + i]);
    }
  }
  expectLen += frame.payloadLen;
  if (buffer_.size() < expectLen) return kNeedMore;
  for (size_t i = 0; i < 4 && frame.mask; ++i) {
    Base::INFO("maskingKey i :{} {}", i,
               static_cast<int>(buffer_[2 + more + i]));
    frame.maskingKey[i] = buffer_[2 + more + i];
  }
  Base::INFO("length:{}", 2 + more + (frame.mask ? 4 : 0));
  std::string_view payload(buffer_.data() + 2 + more + (frame.mask ? 4 : 0),
                           frame.payloadLen);
  Base::INFO("payload:{}", payload);
  frame.payload.reserve(frame.payloadLen);
  if (frame.mask) {
    Base::INFO("do masking");
    for (size_t i = 0; i < frame.payloadLen; ++i) {
      frame.payload.push_back(
          static_cast<char>(payload[i] ^ frame.maskingKey[i % 4]));
      Base::INFO("payload[i]:{}", payload[i]);
      Base::INFO("decode payload[i]:{}",
                 static_cast<char>(payload[i] ^ frame.maskingKey[i % 4]));
    }
  } else {
    frame.payload.append(payload);
  }
  Base::INFO("expectLen:{}", expectLen);
  buffer_.erase(buffer_.begin(),
                buffer_.begin() + static_cast<long>(expectLen));
  if (frame.fin == 1) {
    return kSuccess;
  } else {
    // merge frames
    size_t n = frames_.size() - 1;
    for (size_t i = 0; i < n - 1; ++i) {
      assert(frames_[i].fin == 0);
      assert(frames_[i].opcode == 0);
      frames_[n - 1].payloadLen += frames_[i + 1].payloadLen;
      frames_[n - 1].payload.append(frames_[i + 1].payload);
    }
    frames_[n - 1].fin = 1;
  }
  return kSuccess;
}

void Net::Http::WebSocketParser::MakeFrameToBuffer(
    WebSocketFrame& frame, std::vector<char>& writeBuffer) {
  assert(frame.fin == 1);
  writeBuffer.push_back(static_cast<char>(0x80 | frame.opcode));
  uint8_t maskAndLen = frame.mask ? 0x80 : 0;
  auto size = frame.payloadView.size();
  if (frame.payloadView.size() < 126) {
    maskAndLen |= static_cast<uint8_t>(size);
    writeBuffer.push_back(static_cast<char>(maskAndLen));
  } else if (frame.payloadView.size() < 65536) {
    maskAndLen |= 126;
    writeBuffer.push_back(static_cast<char>(maskAndLen));
    auto ptr = reinterpret_cast<const char*>(&size);
    writeBuffer.insert(writeBuffer.end(), ptr, ptr + 2);
  } else {
    maskAndLen |= 127;
    auto ptr = reinterpret_cast<const char*>(&size);
    writeBuffer.push_back(static_cast<char>(maskAndLen));
    writeBuffer.insert(writeBuffer.end(), ptr, ptr + 8);
  }
  if (frame.mask) {
    uint8_t maskingKey[4];
    for (size_t i = 0; i < 4; ++i) {
      maskingKey[i] = static_cast<uint8_t>(rand() & 0xff);
      writeBuffer.push_back(static_cast<char>(maskingKey[i]));
    }
    for (size_t i = 0; i < frame.payloadView.size(); ++i) {
      writeBuffer.push_back(
          static_cast<char>(frame.payloadView[i] ^ maskingKey[i % 4]));
    }
  } else {
    writeBuffer.insert(writeBuffer.end(), frame.payloadView.begin(),
                       frame.payloadView.end());
  }
}