#include "cold/net/TcpServer.h"
#include "cold/time/Time.h"
#include "cold/time/Timer.h"
#include "third_party/fmt/include/fmt/base.h"

using namespace Cold;

class CharGenServer : public Net::TcpServer {
 public:
  CharGenServer(const Net::IpAddress& addr, size_t poolSize = 0,
                bool print = true, bool reusePort = false,
                bool enableSSL = false)
      : Net::TcpServer(addr, poolSize, reusePort, enableSSL),
        startTime_(Base::Time::Now()),
        timer_(pool_.GetMainIoService()) {
    std::string line;
    for (int i = 33; i < 127; ++i) {
      line.push_back(char(i));
    }
    line += line;
    for (size_t i = 0; i < 127 - 33; ++i) {
      message_ += line.substr(i, 72) + '\n';
    }
    if (print) {
      timer_.ExpiresAfter(std::chrono::seconds(3));
      timer_.AsyncWait(printThroughput());
    }
  }

  ~CharGenServer() override = default;

  Base::Task<> OnConnect(Net::TcpSocket socket) override {
    co_await DoCharGen(std::move(socket));
  }

  Base::Task<> DoCharGen(Net::TcpSocket socket) {
    while (true) {
      auto n = co_await socket.WriteN(message_.data(), message_.size());
      if (n < 0 || static_cast<size_t>(n) != message_.size()) {
        Base::INFO("Connection down. addr: {}",
                   socket.GetRemoteAddress().GetIpPort());
        break;
      }
      transferred_ += message_.size();
    }
  }

  Base::Task<> printThroughput() {
    auto endTime = Base::Time::Now();
    auto d =
        std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime_)
            .count();
    auto mbs = static_cast<double>(transferred_) / static_cast<double>(d) /
               1024 / 1024;
    //保留3位小数 fmt输出
    Base::INFO("Throughput: {:.3f} MB/s", mbs);
    transferred_ = 0;
    startTime_ = endTime;
    timer_.ExpiresAfter(std::chrono::seconds(3));
    timer_.AsyncWait(printThroughput());
    co_return;
  }

  std::string message_;
  int64_t transferred_ = 0;
  Base::Time startTime_;
  Base::Timer timer_;
};

int main() {
  CharGenServer server(Net::IpAddress(8080));
  server.Start();
}