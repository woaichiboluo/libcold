#include "cold/coro/IoContextPool.h"
#include "cold/net/Acceptor.h"
#include "cold/net/TcpSocket.h"

using namespace Cold;

std::string MakeDayTimeString() {
  using namespace std;  // For time_t, time and ctime;
  time_t now = time(0);
  return ctime(&now);
}

class TcpDaytimeServer {
 public:
  TcpDaytimeServer(size_t threadNum, const Net::IpAddress& addr)
      : pool_(threadNum), acceptor_(pool_, addr, true) {}

  void Start() {
    acceptor_.Listen();
    LOG_INFO(Base::GetMainLogger(), "TcpDaytimeServer run at:{}",
             acceptor_.GetLocalAddress().GetIpPort());
    acceptor_.GetIoContext()->CoSpawn(DoAccept());
    pool_.Start();
  }

  Base::Task<> DoAccept() {
    while (true) {
      auto sock = co_await acceptor_.Accept();
      if (sock) sock->GetIoContext()->CoSpawn(DoWrite(std::move(sock.value())));
    }
  }

  Base::Task<> DoWrite(Net::TcpSocket socket) {
    auto str = MakeDayTimeString();
    co_await socket.Write(str.data(), str.size());
    socket.Close();
  }

 private:
  Base::IoContextPool pool_;
  Net::Acceptor acceptor_;
};

int main() {
  Net::IpAddress addr(13);
  TcpDaytimeServer server(4, addr);
  server.Start();
}