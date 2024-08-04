#include "cold/coro/IoService.h"
#include "cold/net/TcpClient.h"
#include "cold/time/Timer.h"

using namespace Cold;

class DaytimeClient : public Net::TcpClient {
 public:
  DaytimeClient(Base::IoService& service, bool enableSSL)
      : Net::TcpClient(service, enableSSL) {}
  ~DaytimeClient() override = default;

  Base::Task<> OnConnect() override {
    Base::INFO("Connect Success. Server address:{}",
               GetSocket().GetRemoteAddress().GetIpPort());
    char buf[256];
    auto readBytes = co_await socket_.Read(buf, sizeof buf);
    if (readBytes <= 0) {
      Base::ERROR("Read failed. ret: {},reason: {}", readBytes,
                  Base::ThisThread::ErrorMsg());
    } else {
      Base::INFO("Server time: {}",
                 std::string(buf, static_cast<size_t>(readBytes)));
    }
    socket_.Close();
    socket_.GetIoService().Stop();
  }
};

int main() {
  Base::IoService service;
  DaytimeClient client(service, true);
  service.CoSpawn(client.Connect(Net::IpAddress(6666)));
  service.Start();
}