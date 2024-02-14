#ifndef COLD_NET_BASICSOCKET
#define COLD_NET_BASICSOCKET

#include <sys/socket.h>

#include <atomic>

#include "cold/net/IoAwaitable.h"
#include "cold/net/IpAddress.h"

namespace Cold::Base {
class IoContext;
}

namespace Cold::Net {

class BasicSocket {
 public:
  explicit BasicSocket(Base::IoContext& ioContext) : ioContext_(&ioContext) {}
  BasicSocket(Base::IoContext& ioContext, int fd)
      : ioContext_(&ioContext), fd_(fd) {}
  BasicSocket(Base::IoContext& ioContext, const IpAddress& local,
              const IpAddress& remote, int fd)
      : ioContext_(&ioContext),
        fd_(fd),
        localAddress_(local),
        remoteAddress_(remote) {}
  virtual ~BasicSocket();

  BasicSocket(const BasicSocket&) = delete;
  BasicSocket& operator=(const BasicSocket&) = delete;

  BasicSocket(BasicSocket&& other);
  BasicSocket& operator=(BasicSocket&& other);

  operator bool() const { return IsValid(); }
  bool IsValid() const { return fd_ >= 0; }
  int NativeHandle() const { return fd_; }

  Base::IoContext* GetIoContext() { return ioContext_; }
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

  auto Connect(const IpAddress& address) {
    return ConnectAwaitable(ioContext_, fd_, address, &connected_,
                            &localAddress_, &remoteAddress_);
  }

  template <typename PERIOD, typename REP>
  auto ConnectWithTimeout(const IpAddress& remoteAddress,
                          std::chrono::duration<PERIOD, REP> duration) {
    return IoTimeoutAwaitable(ioContext_, Connect(remoteAddress), duration);
  }

  auto Read(void* buf, size_t count) {
    return ReadAwaitable(ioContext_, fd_, buf, count, connected_);
  }

  auto Write(const void* buf, size_t count) {
    return WriteAwaitable(ioContext_, fd_, buf, count, connected_);
  }

  template <typename PERIOD, typename REP>
  auto ReadWithTimeout(void* buf, size_t count,
                       std::chrono::duration<PERIOD, REP> duration) {
    return IoTimeoutAwaitable(ioContext_, Read(buf, count), duration);
  }

  template <typename PERIOD, typename REP>
  auto WriteWithTimeout(const void* buf, size_t count,
                        std::chrono::duration<PERIOD, REP> duration) {
    return IoTimeoutAwaitable(ioContext_, Write(buf, count), duration);
  }

 protected:
  Base::IoContext* ioContext_;
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
