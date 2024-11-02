#ifndef COLD_HTTP_WEBSOCKET_WEBSOCKETPARSER
#define COLD_HTTP_WEBSOCKET_WEBSOCKETPARSER

#include <cassert>
#include <queue>
#include <string>
#include <string_view>
#include <vector>

#include "../../util/IntHelper.h"

namespace Cold::Http {

struct WebSocketFrame {
  bool fin;
  bool mask;
  uint8_t opcode;
  uint64_t payloadLen;
  char maskingKey[4];
  std::string payload;
  // for make frame
  std::string_view payloadView;
};

class WebSocketParser {
 public:
  enum ParseState { kSuccess, kError, kNeedMore };
  WebSocketParser() = default;
  ~WebSocketParser() = default;

  WebSocketParser(const WebSocketParser&) = delete;
  WebSocketParser& operator=(const WebSocketParser&) = delete;

  bool Parse(const char* data, size_t len) {
    buffer_.insert(buffer_.end(), data, data + len);
    auto state = DoParse();
    while (state == kSuccess) {
      state = DoParse();
    }
    return state != kError;
  }

  static void MakeFrameToBuffer(WebSocketFrame& frame,
                                std::string& writeBuffer) {
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
      WriteInt(static_cast<uint16_t>(size), std::back_inserter(writeBuffer));
    } else {
      maskAndLen |= 127;
      writeBuffer.push_back(static_cast<char>(maskAndLen));
      WriteInt(static_cast<uint64_t>(size), std::back_inserter(writeBuffer));
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

  bool HasFrame() const { return !completeFrames_.empty(); }

  WebSocketFrame TakeFrame() {
    auto frame = std::move(completeFrames_.front());
    completeFrames_.pop();
    return frame;
  }

 private:
  ParseState DoParse() {
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
    if (more) {
      if (more == 2) {
        frame.payloadLen = ReadInt<uint16_t>(buffer_.data() + 2);
      } else {
        frame.payloadLen = ReadInt<uint64_t>(buffer_.data() + 2);
      }
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

  std::vector<char> buffer_;
  std::queue<WebSocketFrame> completeFrames_;
  std::vector<WebSocketFrame> frames_;
  bool newFrame_ = true;
};

};  // namespace Cold::Http

#endif /* COLD_HTTP_WEBSOCKET_WEBSOCKETPARSER */
