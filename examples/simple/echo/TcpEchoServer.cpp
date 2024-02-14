#include "cold/coro/IoContextPool.h"
#include "cold/net/Acceptor.h"
#include "cold/net/TcpSocket.h"

using namespace Cold;

class EchoServer {
 public:
  EchoServer(size_t threadNum, const Net::IpAddress& addr)
      : pool_(threadNum), acceptor_(pool_, addr, true) {}

  void Start() {
    acceptor_.Listen();
    LOG_INFO(Base::GetMainLogger(), "Server run at:{}",
             acceptor_.GetLocalAddress().GetIpPort());
    acceptor_.GetIoContext()->CoSpawn(DoAccept());
    pool_.Start();
  }

  Base::Task<> DoAccept() {
    while (true) {
      auto sock = co_await acceptor_.Accept();
      if (sock) sock->GetIoContext()->CoSpawn(DoEcho(std::move(sock.value())));
    }
  }

  Base::Task<> DoEcho(Net::TcpSocket socket) {
    while (true) {
      char buf[4096];
      auto readBytes = co_await socket.Read(buf, sizeof buf);
      if (readBytes == 0) {
        socket.Close();
        break;
      } else if (readBytes > 0) {
        std::string_view view{buf, buf + readBytes};
        if (view.back() == '\n') view = view.substr(0, view.size() - 1);
        co_await socket.Write(buf, static_cast<size_t>(readBytes));
      } else {
        LOG_ERROR(Base::GetMainLogger(), "error occurred errno:{},reason:{}",
                  errno, Base::ThisThread::ErrorMsg());
        socket.Close();
        break;
      }
    }
  }

  ~EchoServer() = default;

 private:
  Base::IoContextPool pool_;
  Net::Acceptor acceptor_;
};

int main() {
  Net::IpAddress addr(8888);
  EchoServer server(4, addr);
  server.Start();
}