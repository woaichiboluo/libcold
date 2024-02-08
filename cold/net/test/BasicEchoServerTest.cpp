#include "cold/coro/IoContext.h"
#include "cold/coro/IoContextPool.h"
#include "cold/net/Acceptor.h"
#include "cold/net/TcpSocket.h"

using namespace Cold;

class EchoServer {
 public:
  EchoServer(const Net::IpAddress& listenAddr)
      : ioContextPool_(0), acceptor_(ioContextPool_, listenAddr, true) {}

  void Start() {
    acceptor_.Listen();
    acceptor_.GetIoContext()->CoSpawn(DoAccept());
    ioContextPool_.Start();
  }

  Base::Task<> DoAccept() {
    while (true) {
      auto sock = co_await acceptor_.Accept();
      if (sock) {
        sock->GetIoContext()->CoSpawn(DoEcho(std::move(sock.value())));
      }
    }
  }

  Base::Task<> DoEcho(Net::TcpSocket conn) {
    auto logger = Base::GetMainLogger();
    LOG_INFO(logger, "New connection fd = {} conn = {} <-> {}",
             conn.NativeHandle(), conn.GetLocalAddress().GetIpPort(),
             conn.GetRemoteAddress().GetIpPort());
    while (true) {
      char buf[2048];
      auto readBytes = co_await conn.Read(buf, sizeof buf);
      if (readBytes == 0) {
        LOG_INFO(logger, "Close connection fd = {} conn = {} <-> {}",
                 conn.NativeHandle(), conn.GetLocalAddress().GetIpPort(),
                 conn.GetRemoteAddress().GetIpPort());
        conn.Close();
        break;
      } else if (readBytes > 0) {
        std::string_view view{buf, buf + readBytes};
        if (view.back() == '\n') view = view.substr(0, view.size() - 1);
        LOG_INFO(logger, "Received:{}", view);
        co_await conn.Write(buf, static_cast<size_t>(readBytes));
      } else {
        LOG_ERROR(logger,
                  "Connection error. errno = {} reason = {} fd = {} conn = {} "
                  "<-> {}",
                  errno, Base::ThisThread::ErrorMsg(), conn.NativeHandle(),
                  conn.GetLocalAddress().GetIpPort(),
                  conn.GetRemoteAddress().GetIpPort());
        conn.Close();
        break;
      }
    }
    co_return;
  }

 private:
  Base::IoContextPool ioContextPool_;
  Net::Acceptor acceptor_;
};

int main() {
  EchoServer server(Net::IpAddress(8888));
  server.Start();
}