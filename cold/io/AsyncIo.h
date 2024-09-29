#ifndef COLD_IO_ASYNCIO
#define COLD_IO_ASYNCIO

#include <fcntl.h>

#include "../coroutines/Task.h"
#include "../detail/IoAwaitable.h"
#include "IoContext.h"

namespace Cold {

class IoContext;

/**
 * @brief
 * Async Support for blocking/non-blocking I/O
 * mainly for stdin/stdout/stderr
 * it is also available for raw socket
 * it does not hold the file descriptor
 */

class AsyncIo {
 public:
  // adopt and usage
  AsyncIo(IoContext& context, int fd)
      : context_(&context), event_(context_->TakeIoEvent(fd)) {
    assert(fd >= 0);
  }

  ~AsyncIo() { Clean(); }

  bool IsValid() const { return event_.get(); }

  operator bool() const { return IsValid(); }

  IoContext& GetIoContext() const { return *context_; }

  int GetFd() const {
    assert(IsValid());
    return event_->GetFd();
  }

  [[nodiscard]] Task<ssize_t> AsyncReadSome(void* buf, size_t count) {
    co_await Detail::ReadIoAwaitable(event_, true);
    auto fd = event_->GetFd();
    co_return read(fd, buf, count);
  }

  [[nodiscard]] Task<ssize_t> AsyncWriteSome(const void* buf, size_t count) {
    auto fd = event_->GetFd();
    while (true) {
      auto ret = write(fd, buf, count);
      if (ret >= 0 || errno != EAGAIN) {
        co_return ret;
      }
      co_await Detail::ReadIoAwaitable(event_, true);
    }
  }

  [[nodiscard]] Task<ssize_t> AsyncReadN(void* buf, size_t size) {
    size_t byteAlreadyRead = 0;
    while (byteAlreadyRead < size) {
      ssize_t ret = co_await AsyncReadSome(
          static_cast<char*>(buf) + byteAlreadyRead, size - byteAlreadyRead);
      if (ret <= 0) co_return ret;
      byteAlreadyRead += static_cast<size_t>(ret);
    }
    co_return static_cast<ssize_t>(size);
  }

  [[nodiscard]] Task<ssize_t> AsyncWriteN(const void* buf, size_t size) {
    size_t byteAlreadyWrite = 0;
    while (byteAlreadyWrite < size) {
      ssize_t ret = co_await AsyncWriteSome(
          static_cast<const char*>(buf) + byteAlreadyWrite,
          size - byteAlreadyWrite);
      if (ret <= 0) co_return ret;
      byteAlreadyWrite += static_cast<size_t>(ret);
    }
    co_return static_cast<ssize_t>(size);
  }

  int Release() {
    auto fd = event_->GetFd();
    Clean();
    return fd;
  }

 private:
  void Clean() {
    if (!event_) return;
    event_->ReturnIoEvent();
  }

  IoContext* context_;
  std::shared_ptr<Detail::IoEvent> event_;
};

}  // namespace Cold

#endif /* COLD_IO_ASYNCIO */
