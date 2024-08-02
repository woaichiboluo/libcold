#ifndef COLD_CORO_IO
#define COLD_CORO_IO

#include <fcntl.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>

#include "cold/coro/IoService.h"
#include "cold/log/Logger.h"
#include "cold/time/Timer.h"

namespace Cold::Base {

class IoAwaitableBase {
  template <typename AWAITABLE, typename REP, typename PERIOD>
  friend class IoTimeoutAwaitable;

 public:
  enum IoType { kREAD, kWRITE };
  IoAwaitableBase(Base::IoService* service, int fd, IoType type)
      : service_(service), fd_(fd), type_(type) {}

  virtual ~IoAwaitableBase() = default;

  void await_suspend(std::coroutine_handle<> handle) noexcept {
    ListenIo(handle);
  }

 protected:
  void ListenIo(const std::coroutine_handle<>& handle) {
    if (type_ == kREAD) {
      service_->ListenReadEvent(fd_, handle);
    } else {
      service_->ListenWriteEvent(fd_, handle);
    }
  }

  void StopListeningIo() {
    if (type_ == kREAD) {
      service_->StopListeningReadEvent(fd_);
    } else {
      service_->StopListeningWriteEvent(fd_);
    }
  }

  void SetIoType(IoType type) { type_ = type; }

  bool GetTimeout() const { return timeout_; }

  Base::IoService* service_;
  int fd_;

 private:
  void SetTimeout() {
    timeout_ = true;
    StopListeningIo();
  }

  IoType type_;
  bool timeout_ = false;
};

template <typename AWAITABLE, typename REP, typename PERIOD>
class IoTimeoutAwaitable {
 public:
  using Duration = std::chrono::duration<REP, PERIOD>;
  using RetType =
      std::invoke_result_t<decltype(&AWAITABLE::await_resume), AWAITABLE>;

  IoTimeoutAwaitable(Base::IoService* service, AWAITABLE&& awaitable,
                     Duration timeoutTime)
      : service_(service),
        awaitable_(std::move(awaitable)),
        timeoutTime_(timeoutTime),
        timer_(*service) {}
  ~IoTimeoutAwaitable() = default;

  bool await_ready() noexcept { return false; }

  void await_suspend(std::coroutine_handle<> handle) noexcept {
    auto task = [](std::coroutine_handle<> coro, RetType& retValue,
                   AWAITABLE& awaitable) -> Base::Task<> {
      retValue = co_await awaitable;
      coro.resume();
    }(handle, retValue_, awaitable_);
    timer_.ExpiresAfter(timeoutTime_);
    timer_.AsyncWait(
        [](std::coroutine_handle<> coro, AWAITABLE& awaitable) -> Base::Task<> {
          awaitable.SetTimeout();
          coro.resume();
          co_return;
        }(task.GetHandle(), awaitable_));
    service_->CoSpawn(std::move(task));
  }

  RetType await_resume() noexcept {
    if (!awaitable_.GetTimeout()) timer_.Cancel();
    return retValue_;
  }

 private:
  Base::IoService* service_;
  AWAITABLE awaitable_;
  Duration timeoutTime_;
  Base::Timer timer_;
  RetType retValue_;
};

namespace detail {
class AsyncRead : public IoAwaitableBase {
 public:
  AsyncRead(IoService& service, int fd, void* ptr, size_t readSize)
      : IoAwaitableBase(&service, fd, IoType::kREAD),
        ptr_(ptr),
        readSize_(readSize) {}
  ~AsyncRead() override = default;

  bool await_ready() noexcept { return false; }

  ssize_t await_resume() noexcept {
    if (GetTimeout()) {
      errno = ETIMEDOUT;
      return -1;
    }
    return read(fd_, ptr_, readSize_);
  }

 private:
  void* ptr_;
  size_t readSize_;
};

class AsyncWrite : public IoAwaitableBase {
 public:
  AsyncWrite(IoService& service, int fd, const void* ptr, size_t writeSize)
      : IoAwaitableBase(&service, fd, IoType::kWRITE),
        ptr_(ptr),
        writeSize_(writeSize) {}

  ~AsyncWrite() override = default;

  bool await_ready() noexcept { return false; }

  ssize_t await_resume() noexcept {
    if (GetTimeout()) {
      errno = ETIMEDOUT;
      return -1;
    }
    return write(fd_, ptr_, writeSize_);
  }

 private:
  const void* ptr_;
  size_t writeSize_;
};

}  // namespace detail

class AsyncIO {
 public:
  enum class Mode {
    kRead = O_RDONLY,
    kWrite = O_WRONLY | O_CREAT | O_TRUNC,
    kAppend = O_WRONLY | O_CREAT | O_APPEND,
    kReadWrite = O_RDWR,
    kReadWriteTrunc = O_RDWR | O_CREAT | O_TRUNC,
    kReadAppen = O_RDWR | O_CREAT | O_APPEND
  };

  AsyncIO(IoService& service, std::string filename, Mode mode)
      : service_(&service), filename_(std::move(filename)), mode_(mode) {
    fd_ = open(filename_.c_str(), static_cast<int>(mode_));
    if (fd_ < 0) {
      Base::ERROR("open file {} failed. reason: {}", filename_,
                  Base::ThisThread::ErrorMsg());
    }
  }

  AsyncIO(IoService& service, int fd, bool adopt = true)
      : service_(&service), fd_(fd), adopt_(adopt) {}

  ~AsyncIO() {
    if (fd_ && !adopt_) {
      service_->StopListeningAll(fd_);
      close(fd_);
    }
  }

  AsyncIO(const AsyncIO&) = delete;
  AsyncIO& operator=(const AsyncIO&) = delete;

  AsyncIO(AsyncIO&& other)
      : service_(other.service_),
        filename_(std::move(other.filename_)),
        mode_(other.mode_),
        fd_(other.fd_),
        adopt_(other.adopt_) {
    other.fd_ = -1;
  }

  AsyncIO& operator=(AsyncIO&& other) {
    if (this == &other) return *this;
    if (fd_ && !adopt_) {
      service_->StopListeningAll(fd_);
      close(fd_);
    }
    service_ = other.service_;
    filename_ = std::move(other.filename_);
    fd_ = other.fd_;
    other.fd_ = -1;
    mode_ = other.mode_;
    adopt_ = other.adopt_;
    return *this;
  }

  void Swap(AsyncIO& other) {
    std::swap(service_, other.service_);
    std::swap(filename_, other.filename_);
    std::swap(fd_, other.fd_);
    std::swap(mode_, other.mode_);
    std::swap(adopt_, other.adopt_);
  }

  bool reOpen(std::string filename, Mode mode) {
    AsyncIO tmp(*service_, std::move(filename), mode);
    Swap(tmp);
    return *this;
  }

  bool IsOpen() const { return fd_ >= 0; }
  operator bool() const { return IsOpen(); }

  int GetFd() { return fd_; }
  Mode GetMode() const { return mode_; }
  IoService* GetService() const { return service_; }

  [[nodiscard]] auto AsyncRead(void* ptr, size_t len) {
    return detail::AsyncRead(*service_, fd_, ptr, len);
  }

  [[nodiscard]] auto AsyncWrite(const void* ptr, size_t len) {
    return detail::AsyncWrite(*service_, fd_, ptr, len);
  }

  template <typename REP, typename PERIOD>
  [[nodiscard]] auto AsyncReadWithTimeout(
      void* ptr, size_t len, std::chrono::duration<REP, PERIOD> d) {
    return IoTimeoutAwaitable(service_, AsyncRead(ptr, len), d);
  }

  template <typename REP, typename PERIOD>
  [[nodiscard]] auto AsyncWriteWithTimeout(
      void* ptr, size_t len, std::chrono::duration<REP, PERIOD> d) {
    return IoTimeoutAwaitable(service_, AsyncWrite(ptr, len), d);
  }

  void Close() {
    if (fd_) {
      service_->StopListeningAll(fd_);
      close(fd_);
      fd_ = -1;
    }
  }

 private:
  IoService* service_;
  std::string filename_;
  Mode mode_;
  int fd_ = -1;
  bool adopt_ = false;
};

}  // namespace Cold::Base

#endif /* COLD_CORO_IO */
