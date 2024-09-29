#include "cold/Cold.h"

using namespace Cold;

class ChargenServer : public TcpServer {
 public:
  ChargenServer(const IpAddress& addr, size_t poolSize = 0, bool print = true)
      : TcpServer(addr, poolSize, "Chargen"), timer_(pool_.GetMainIoContext()) {
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
      timer_.AsyncWait([&]() { PrintThroughput(); });
    }
  }
  ~ChargenServer() override = default;

  Task<> DoChargen(TcpSocket socket) {
    while (true) {
      auto n = co_await socket.WriteN(message_.data(), message_.size());
      if (n < 0 || static_cast<size_t>(n) != message_.size()) {
        INFO("Connection down. addr: {}",
             socket.GetRemoteAddress().GetIpPort());
        break;
      }
      bytesTransferred_ += message_.size();
    }
  }

  Task<> OnNewConnection(TcpSocket socket) override {
    co_await DoChargen(std::move(socket));
  }

 private:
  void PrintThroughput() {
    auto endTime = Time::Now();
    auto secs =
        std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime_)
            .count();
    double mibs = static_cast<double>(bytesTransferred_) /
                  static_cast<double>(secs) / 1024 / 1024;
    double mbs = static_cast<double>(bytesTransferred_) /
                 static_cast<double>(secs) / 1000 / 1000;
    INFO("Throughput: {:.3f} MiB/s {:.3f} MB/s", mibs, mbs);
    bytesTransferred_ = 0;
    startTime_ = endTime;
    timer_.ExpiresAfter(std::chrono::seconds(3));
    timer_.AsyncWait([&]() { PrintThroughput(); });
  }

  Timer timer_;
  std::string message_;
  int64_t bytesTransferred_ = 0;
  Time startTime_;
};

int main() {
  ChargenServer server(IpAddress(8888));
  INFO("Chargen server started at: {}", server.GetLocalAddress().GetIpPort());
  server.Start();
}