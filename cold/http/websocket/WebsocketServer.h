#ifndef COLD_HTTP_WEBSOCKET_WEBSOCKETSERVER
#define COLD_HTTP_WEBSOCKET_WEBSOCKETSERVER

#include "../../util/Base64.h"
#include "../../util/Crypto.h"
#include "../../util/Url.h"
#include "../HttpResponse.h"
#include "../RawHttpRequest.h"
#include "WebSocket.h"

#ifndef COLD_ENABLE_SSL
#ERROR "WebSocketServer requires SSL enabled"
#endif

namespace Cold::Http {

class WebSocket;

using WebSocketPtr = std::shared_ptr<WebSocket>;

class WebSocketServer {
 public:
  WebSocketServer(std::string url) : url_(std::move(url)) {
    onConnect_ = [](std::shared_ptr<WebSocket>) {
      INFO("new WebSocket connected");
    };
  }

  WebSocketServer(const WebSocketServer&) = delete;
  WebSocketServer& operator=(const WebSocketServer&) = delete;
  ~WebSocketServer() = default;

  static bool CheckWhetherUpgradeRequest(const RawHttpRequest& request) {
    return request.GetHeader("Connection") == "Upgrade" &&
           request.GetHeader("Upgrade") == "websocket" &&
           request.HasHeader("Sec-WebSocket-Key");
  }

  const std::string& GetUrl() const { return url_; }

  void SetOnConnect(std::function<void(std::shared_ptr<WebSocket>)> onConnect) {
    onConnect_ = std::move(onConnect);
  }

  Task<> OnReceivedUpgradeRequest(RawHttpRequest request, TcpSocket socket) {
    auto websocketKey = request.GetHeader("Sec-WebSocket-Key");
    auto key =
        std::string(websocketKey) + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    auto encodedKey = Base64Encode(SHA1(key));
    HttpResponse response;
    response.SetHttpStatusCode(k101);
    response.SetHttpConnectionStatus(kUpgrade);
    response.SetHeader("Upgrade", "websocket");
    response.SetHeader("Sec-WebSocket-Accept", encodedKey);
    std::string headerBuf;
    response.MakeHeaders(headerBuf);
    INFO("socket: {} iocontext: {}", socket.IsConnected(),
         reinterpret_cast<uintptr_t>(&socket.GetIoContext()));
    if (co_await socket.Write(headerBuf.data(), headerBuf.size()) !=
        static_cast<ssize_t>(headerBuf.size())) {
      co_return;
    }
    std::shared_ptr<WebSocket> wb = std::make_shared<WebSocket>(
        UrlDecode(request.GetUrl()), std::move(socket));
    wb->GetIoContext().CoSpawn(wb->DoRead(wb));
    wb->GetIoContext().CoSpawn(wb->DoWrite(wb));
    if (onConnect_) onConnect_(wb);
  }

 private:
  std::string url_;

  std::function<void(std::shared_ptr<WebSocket>)> onConnect_;
};

}  // namespace Cold::Http

#endif /* COLD_HTTP_WEBSOCKET_WEBSOCKETSERVER */
