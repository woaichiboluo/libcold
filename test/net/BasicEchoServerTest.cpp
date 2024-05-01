#include <sys/resource.h>

#include "cold/coro/IoService.h"
#include "cold/log/Logger.h"
#include "cold/net/Acceptor.h"
#include "cold/net/TcpSocket.h"

using namespace Cold;

class EchoServer {
 public:
  EchoServer(const Net::IpAddress& listenAddr)
      : acceptor_(service_, listenAddr, true) {}

  void Start() {
    acceptor_.Listen();
    acceptor_.GetIoService().CoSpawn(DoAccept());
    service_.Start();
  }

  Base::Task<> DoAccept() {
    while (true) {
      auto sock = co_await acceptor_.Accept();
      if (sock) {
        sock.GetIoService().CoSpawn(DoEcho(std::move(sock)));
      }
    }
  }

  Base::Task<> DoEcho(Net::TcpSocket conn) {
    Base::INFO("New connection fd: {}, conn: {} <-> {} ", conn.NativeHandle(),
               conn.GetLocalAddress().GetIpPort(),
               conn.GetRemoteAddress().GetIpPort());
    while (true) {
      char buf[2048];
      auto readBytes = co_await conn.Read(buf, sizeof buf);
      if (readBytes == 0) {
        Base::INFO("Close connection fd: {}, conn: {} <-> {}",
                   conn.NativeHandle(), conn.GetLocalAddress().GetIpPort(),
                   conn.GetRemoteAddress().GetIpPort());
        conn.Close();
        break;
      } else if (readBytes > 0) {
        std::string_view view{buf, buf + readBytes};
        if (view.back() == '\n') view = view.substr(0, view.size() - 1);
        Base::INFO("Received:{}", view);
        co_await conn.Write(buf, static_cast<size_t>(readBytes));
      } else {
        Base::ERROR(
            "Connection error. errno: {}, reason: {}, fd: {}, conn: {} "
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
  Base::IoService service_;
  Net::Acceptor acceptor_;
};

int main() {
  Base::LogManager::Instance().GetMainLoggerRaw()->SetLevel(
      Base::LogLevel::TRACE);
  EchoServer server(Net::IpAddress(8888));
  Base::INFO("Server run at port 8888");
  server.Start();
}