#ifndef NET_HTTP_WEBSOCKETPARSER
#define NET_HTTP_WEBSOCKETPARSER

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

  ParseState Parse(const char* data, size_t len);

  static void MakeFrameToBuffer(WebSocketFrame& frame,
                                std::vector<char>& writeBuffer);

  WebSocketFrame TakeFrame() {
    auto frame = std::move(frames_.back());
    frames_.pop_back();
    return frame;
  }

 private:
  std::vector<char> buffer_;
  std::vector<WebSocketFrame> frames_;
};
};  // namespace Cold::Net::Http

#endif /* NET_HTTP_WEBSOCKETPARSER */
