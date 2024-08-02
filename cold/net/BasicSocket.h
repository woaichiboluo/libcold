#ifndef COLD_NET_BASICSOCKET
#define COLD_NET_BASICSOCKET

#include <sys/socket.h>

#include <atomic>

#include "cold/net/IoAwaitable.h"
#include "cold/net/IpAddress.h"

#ifdef COLD_NET_ENABLE_SSL
#include "cold/net/ssl/SSLContext.h"
#endif

namespace Cold::Net {

class BasicSocket {
 public:
  BasicSocket() : ioService_(nullptr) {}

  explicit BasicSocket(Base::IoService& service) : ioService_(&service) {
    (void)ssl_;
  }

  BasicSocket(Base::IoService& service, int fd, bool enableSSL)
      : ioService_(&service), fd_(fd) {
#ifdef COLD_NET_ENABLE_SSL
    if (enableSSL) {
      enableSSL_ = true;
      ssl_ = SSL_new(SSLContext::GetInstance().GetContext());
      if (!ssl_) {
        Base::ERROR("SSL_new failed");
        return;
      }
      if (SSL_set_fd(ssl_, fd_) == 0) {
        Base::ERROR("SSL_set_fd failed");
        return;
      }
    }
#endif
  }

  BasicSocket(Base::IoService& service, const IpAddress& local,
              const IpAddress& remote, int fd, SSL* ssl)
      : ioService_(&service),
        fd_(fd),
        localAddress_(local),
        remoteAddress_(remote),
        ssl_(ssl) {}

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

  [[nodiscard]] auto Read(void* buf, size_t count) {
    return ReadAwaitable(ioService_, fd_, buf, count, connected_, ssl_);
  }

  [[nodiscard]] auto Write(const void* buf, size_t count) {
    return WriteAwaitable(ioService_, fd_, buf, count, connected_, ssl_);
  }

  [[nodiscard]] auto Connect(const IpAddress& address) {
    return ConnectAwaitable(ioService_, fd_, address, &connected_,
                            &localAddress_, &remoteAddress_);
  }

  template <typename REP, typename PERIOD>
  [[nodiscard]] auto ReadWithTimeout(
      void* buf, size_t count, std::chrono::duration<REP, PERIOD> duration) {
    return IoTimeoutAwaitable(ioService_, Read(buf, count), duration);
  }

  template <typename REP, typename PERIOD>
  [[nodiscard]] auto WriteWithTimeout(
      const void* buf, size_t count,
      std::chrono::duration<REP, PERIOD> duration) {
    return IoTimeoutAwaitable(ioService_, Write(buf, count), duration);
  }

  template <typename REP, typename PERIOD>
  [[nodiscard]] auto ConnectWithTimeout(
      const IpAddress& remoteAddress,
      std::chrono::duration<REP, PERIOD> duration) {
    return IoTimeoutAwaitable(ioService_, Connect(remoteAddress), duration);
  }

  bool IsEnableSSL() const { return enableSSL_; }

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
  SSL* ssl_ = nullptr;
  bool enableSSL_ = false;
};

}  // namespace Cold::Net

#endif /* COLD_NET_BASICSOCKET */
