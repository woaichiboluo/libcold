#ifndef NET_HTTP_WEBSOCKETSERVER
#define NET_HTTP_WEBSOCKETSERVER

#include <unordered_set>

#include "cold/net/TcpServer.h"
#include "cold/net/http/RawHttpRequest.h"

namespace Cold::Net::Http {

class WebSocket;
using WebSocketPtr = std::shared_ptr<WebSocket>;

class WebSocketServer {
  class StandAloneWsServerContext : public TcpServer {
    friend class WebSocketServer;

   public:
    StandAloneWsServerContext(const Net::IpAddress& addr, size_t poolSize = 0,
                              bool reusePort = false, bool enableSSL = false)
        : TcpServer(addr, poolSize, reusePort, enableSSL) {}
    ~StandAloneWsServerContext() = default;

    Base::Task<> OnConnect(TcpSocket socket) override;

   private:
    std::function<void(RawHttpRequest, TcpSocket)> onUpgradeRequest_;
  };

  using WebSocketPtr = std::shared_ptr<WebSocket>;

 public:
  // for compose with HttpServer
  WebSocketServer() = default;
  // for stand-alone usage
  explicit WebSocketServer(const Net::IpAddress& addr, size_t poolSize = 0,
                           bool reusePort = false, bool enableSSL = false);

  virtual ~WebSocketServer() = default;

  bool CheckWhetherUpgradeRequest(RawHttpRequest request) const;

  Base::Task<> OnReceivedUpgradeRequest(RawHttpRequest request,
                                        TcpSocket socket);

  virtual void OnNewWebSocket(WebSocketPtr ws) {
    Base::INFO("New WebSocket connected");
  }

  void Start() {
    assert(wsServer_);
    wsServer_->Start();
  }

 protected:
  std::unique_ptr<StandAloneWsServerContext> wsServer_;
  Base::Mutex mutex_;
  std::unordered_set<WebSocketPtr> webSockets_ GUARDED_BY(mutex_);
};

}  // namespace Cold::Net::Http

#endif /* NET_HTTP_WEBSOCKETSERVER */
