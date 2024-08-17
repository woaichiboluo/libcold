#ifndef NET_HTTP_WEBSOCKETPARSER
#define NET_HTTP_WEBSOCKETPARSER

#include <queue>
#include <string>
#include <vector>

namespace Cold::Net::Http {

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

  bool Parse(const char* data, size_t len);

  static void MakeFrameToBuffer(WebSocketFrame& frame,
                                std::vector<char>& writeBuffer);

  bool HasFrame() const { return !completeFrames_.empty(); }

  WebSocketFrame TakeFrame() {
    auto frame = std::move(completeFrames_.front());
    completeFrames_.pop();
    return frame;
  }

 private:
  ParseState DoParse();
  std::vector<char> buffer_;
  std::queue<WebSocketFrame> completeFrames_;
  std::vector<WebSocketFrame> frames_;
  bool newFrame_ = true;
};
};  // namespace Cold::Net::Http

#endif /* NET_HTTP_WEBSOCKETPARSER */
