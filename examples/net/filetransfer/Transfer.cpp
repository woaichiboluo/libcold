#include "cold/Cold.h"

using namespace Cold;

class Transfer : public TcpServer {
 public:
  Transfer(std::string fileName, IpAddress addr, size_t poolsize = 0)
      : TcpServer(addr, poolsize), fileName_(std::move(fileName)) {}

 private:
  Task<> SendFile(TcpSocket socket) {
    auto fp = fopen(fileName_.c_str(), "rb");
    ScopeGuard guard([fp]() {
      if (fp) fclose(fp);
    });
    if (!fp) {
      ERROR("failed to open file {}", fileName_);
      socket.Close();
      co_return;
    }
    char buf[64 * 1024];
    while (true) {
      size_t n = fread(buf, 1, sizeof(buf), fp);
      if (n == 0) {
        break;
      }
      ssize_t ret = co_await socket.WriteN(buf, n);
      if (static_cast<ssize_t>(n) != ret) {
        ERROR("failed to write to socket: {}. reason: {}", ret,
              ThisThread::ErrorMsg());
        break;
      }
    }
    socket.Close();
  }

  Task<> OnNewConnection(TcpSocket socket) override {
    INFO("new connection from {}", socket.GetRemoteAddress().GetIpPort());
    co_await SendFile(std::move(socket));
  }

  std::string fileName_;
};

int main(int argc, char** argv) {
  if (argc != 2) {
    fmt::println("Usage: {} <filename>", argv[0]);
    return 1;
  }
  Transfer server(argv[1], IpAddress(8888), 4);
  server.Start();
}