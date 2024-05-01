#ifndef COLD_NET_BASICSOCKET
#define COLD_NET_BASICSOCKET

#include <sys/socket.h>

#include <atomic>

#include "cold/coro/IoService.h"
#include "cold/net/IoAwaitable.h"
#include "cold/net/IpAddress.h"

namespace Cold::Base {
class IoService;
}

namespace Cold::Net {

class BasicSocket {
 public:
  BasicSocket() : ioService_(nullptr) {}

  explicit BasicSocket(Base::IoService& service) : ioService_(&service) {}

  BasicSocket(Base::IoService& service, int fd)
      : ioService_(&service), fd_(fd) {}

  BasicSocket(Base::IoService& service, const IpAddress& local,
              const IpAddress& remote, int fd)
      : ioService_(&service),
        fd_(fd),
        localAddress_(local),
        remoteAddress_(remote) {}

  virtual ~BasicSocket();

  BasicSocket(const BasicSocket&) = delete;
  BasicSocket& operator=(const BasicSocket&) = delete;

  BasicSocket(BasicSocket&& other);
  BasicSocket& operator=(BasicSocket&& other);

  bool IsValid() const { return ioService_ && fd_ >= 0; }

  operator bool() const { return IsValid(); }

  int NativeHandle() const { return fd_; }

  Base::IoService& GetIoService() { return *ioService_; }

  IpAddress GetLocalAddress() const { return localAddress_; }
  IpAddress GetRemoteAddress() const { return remoteAddress_; }

  void SetLocalAddress(const IpAddress& addr) { localAddress_ = addr; }
  void SetRemoteAddress(const IpAddress& addr) { remoteAddress_ = addr; }

  bool Bind(IpAddress address);
  void ShutDown();

  virtual void Close();
  bool IsConnected() const { return connected_; }

  template <typename SocketOption>
  bool SetOption(const SocketOption& option) const {
    return setsockopt(fd_, option.level, option.optName, &option.value,
                      option.len) == 0;
  }

  template <typename SocketOption>
  bool GetOption(SocketOption& option) const {
    return getsockopt(fd_, option.level, option.optName, &option.value,
                      &option.len) == 0;
  }

  auto Read(void* buf, size_t count) {
    return ReadAwaitable(ioService_, fd_, buf, count, connected_);
  }

  auto Write(const void* buf, size_t count) {
    return WriteAwaitable(ioService_, fd_, buf, count, connected_);
  }

  auto Connect(const IpAddress& address) {
    return ConnectAwaitable(ioService_, fd_, address, &connected_,
                            &localAddress_, &remoteAddress_);
  }

 protected:
  Base::IoService* ioService_;
  int fd_ = -1;
  IpAddress localAddress_;
  IpAddress remoteAddress_;
  /*
    针对TCP socket而言 connected_代表的是两个TCP连接之间是否建立有连接
    针对UDP socket而言 connected_代表的是UDP socket是否使用了Connect
  */
  std::atomic<bool> connected_ = false;
};

}  // namespace Cold::Net

#endif /* COLD_NET_BASICSOCKET */
