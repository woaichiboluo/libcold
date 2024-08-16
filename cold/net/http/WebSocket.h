#ifndef NET_HTTP_WEBSOCKET
#define NET_HTTP_WEBSOCKET

#include "cold/net/TcpSocket.h"

namespace Cold::Net::Http {

class WebSocket : public std::enable_shared_from_this<WebSocket> {
  friend class WebSocketServer;

 public:
  using WebSocketPtr = std::shared_ptr<WebSocket>;
  WebSocket(std::string requestUrl, TcpSocket socket)
      : requestUrl_(std::move(requestUrl)), socket_(std::move(socket)) {
    onRecv_ = [](WebSocketPtr sock, const char* data, size_t len) {
      Base::INFO("WebSocket recv: ", std::string_view(data, len));
    };
  }

  ~WebSocket() = default;

  WebSocket(const WebSocket&) = delete;
  WebSocket& operator=(const WebSocket&) = delete;

  void SetOnRecv(
      std::function<void(WebSocketPtr, const char* data, size_t len)> onRecv) {
    onRecv_ = std::move(onRecv);
  }

  void Send(const char* data, size_t len, bool binary = false);
  void Close();

  std::string_view GetRequestUrl() const { return requestUrl_; }

 private:
  // call by server not user
  Base::Task<> DoRead();
  Base::Task<> DoWrite();
  void SendPong();
  void OnError();

  std::string requestUrl_;
  TcpSocket socket_;

  std::function<void(WebSocketPtr)> onClose_;
  std::function<void(WebSocketPtr, const char* data, size_t len)> onRecv_;

  bool isWriting_ = false;
  std::vector<char> writeTempBuffer_;
  std::vector<char> writeBuffer_;
};

}  // namespace Cold::Net::Http

#endif /* NET_HTTP_WEBSOCKET */