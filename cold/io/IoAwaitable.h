#ifndef COLD_IO_IOAWAITABLE
#define COLD_IO_IOAWAITABLE

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <memory>

#include "IoEvent.h"

namespace Cold::detail {

// for non-blocking io
// mainly for the socket
class IoAwaitableBaseForET {
 public:
  IoAwaitableBaseForET(IoEvent* ioEvent, bool ioForRead) noexcept
      : ioEvent_(ioEvent), ioForRead_(ioForRead) {}
  virtual ~IoAwaitableBaseForET() = default;

  IoAwaitableBaseForET(const IoAwaitableBaseForET&) = delete;
  IoAwaitableBaseForET& operator=(const IoAwaitableBaseForET&) = delete;
  IoAwaitableBaseForET(IoAwaitableBaseForET&&) = default;
  IoAwaitableBaseForET& operator=(IoAwaitableBaseForET&&) = default;

  void await_suspend(std::coroutine_handle<> handle) noexcept {
    if (ioForRead_) {
      ioEvent_->SetOnReadCoroutine(handle);
    } else {
      ioEvent_->SetOnWriteCoroutine(handle);
      ioEvent_->EnableWritingET();
    }
  }

  void OnTimeout() {
    if (ioForRead_) {
      ioEvent_->ClearReadCoroutine();
    } else {
      ioEvent_->ClearWriteCoroutine();
    }
  }

 protected:
  IoEvent* ioEvent_;
  bool ioForRead_;
};

// for blocking non-blocking io
// so when await_ready,always watch the io event
// when solve the file and stdio, we use the blocking io,so use LT mode
class IoAwaitableBaseForLT {
 public:
  IoAwaitableBaseForLT(IoEvent* ioEvent, bool read)
      : ioEvent_(ioEvent), ioForRead_(read) {}
  virtual ~IoAwaitableBaseForLT() = default;

  IoAwaitableBaseForLT(const IoAwaitableBaseForLT&) = delete;
  IoAwaitableBaseForLT& operator=(const IoAwaitableBaseForLT&) = delete;
  IoAwaitableBaseForLT(IoAwaitableBaseForLT&&) = default;
  IoAwaitableBaseForLT& operator=(IoAwaitableBaseForLT&&) = default;

  bool await_ready() noexcept { return false; }

  void await_suspend(std::coroutine_handle<> handle) noexcept {
    if (ioForRead_) {
      ioEvent_->SetOnReadCoroutine(handle);
      ioEvent_->EnableReading();
    } else {
      ioEvent_->SetOnWriteCoroutine(handle);
      ioEvent_->EnableWriting();
    }
  }

  void OnTimeout() {
    if (ioForRead_) {
      ioEvent_->DisableReading();
    } else {
      ioEvent_->DisableWriting();
    }
  }

 protected:
  IoEvent* ioEvent_;
  bool ioForRead_;
};

class ReadAwaitableForLT : public IoAwaitableBaseForLT {
 public:
  ReadAwaitableForLT(IoEvent* ioEvent, void* buf, size_t count)
      : IoAwaitableBaseForLT(ioEvent, true), buf_(buf), count_(count) {}
  ~ReadAwaitableForLT() override = default;

  ssize_t await_resume() noexcept {
    return read(ioEvent_->GetFd(), buf_, count_);
  }

 private:
  void* buf_;
  size_t count_;
};

class WriteAwaitableForLT : public IoAwaitableBaseForLT {
 public:
  WriteAwaitableForLT(IoEvent* ioEvent, const void* buf, size_t count)
      : IoAwaitableBaseForLT(ioEvent, false), buf_(buf), count_(count) {}
  ~WriteAwaitableForLT() override = default;

  ssize_t await_resume() noexcept {
    return write(ioEvent_->GetFd(), buf_, count_);
  }

 private:
  const void* buf_;
  size_t count_;
};

class IReadAwaitable : public IoAwaitableBaseForET {
 public:
  IReadAwaitable(IoEvent* ioEvent, bool canReading, void* buf, size_t count)
      : IoAwaitableBaseForET(ioEvent, true),
        canReading_(canReading),
        buf_(buf),
        count_(count) {}
  ~IReadAwaitable() override = default;

  virtual bool await_ready() noexcept = 0;
  virtual ssize_t await_resume() noexcept = 0;

 protected:
  bool canReading_;
  void* buf_;
  size_t count_;
  ssize_t retValue_ = -1;
};

class ReadAwaitableImpl : public IReadAwaitable {
 public:
  ReadAwaitableImpl(IoEvent* ioEvent, bool canReading, void* buf, size_t count)
      : IReadAwaitable(ioEvent, canReading, buf, count) {}
  ~ReadAwaitableImpl() override = default;

  bool await_ready() noexcept override {
    if (!canReading_) return true;
    retValue_ = read(ioEvent_->GetFd(), buf_, count_);
    retReady_ = retValue_ >= 0 || (retValue_ == -1 && errno != EAGAIN);
    return retReady_;
  }

  ssize_t await_resume() noexcept override {
    if (!canReading_) {  // not connected
      errno = ESHUTDOWN;
      return -1;
    } else if (!retReady_) {
      retValue_ = read(ioEvent_->GetFd(), buf_, count_);
    }
    return retValue_;
  }

 private:
  bool retReady_ = false;
};

// FIXME implement it later
class SSLReadAwaitableImpl;

class ReadAwaitable {
 public:
  ReadAwaitable(IoEvent* ioEvent, bool canReading, void* buf, size_t count,
                bool ssl)
      : impl_(ssl ? std::make_unique<ReadAwaitableImpl>(ioEvent, canReading,
                                                        buf, count)
                  : std::make_unique<ReadAwaitableImpl>(ioEvent, canReading,
                                                        buf, count)) {}
  ~ReadAwaitable() = default;

  ReadAwaitable(const ReadAwaitable&) = delete;
  ReadAwaitable& operator=(const ReadAwaitable&) = delete;
  ReadAwaitable(ReadAwaitable&&) = default;
  ReadAwaitable& operator=(ReadAwaitable&&) = default;

  bool await_ready() noexcept { return impl_->await_ready(); }
  void await_suspend(std::coroutine_handle<> handle) noexcept {
    impl_->await_suspend(handle);
  }
  ssize_t await_resume() noexcept { return impl_->await_resume(); }

 private:
  std::unique_ptr<IReadAwaitable> impl_;
};

class IWriteAwaitable : public IoAwaitableBaseForET {
 public:
  IWriteAwaitable(IoEvent* ioEvent, bool canWriting, const void* buf,
                  size_t count)
      : IoAwaitableBaseForET(ioEvent, false),
        canWriting_(canWriting),
        buf_(buf),
        count_(count) {}
  ~IWriteAwaitable() override = default;

  virtual bool await_ready() noexcept = 0;
  virtual ssize_t await_resume() noexcept = 0;

 protected:
  bool canWriting_;
  const void* buf_;
  size_t count_;
  ssize_t retValue_ = -1;
};

class WriteAwaitableImpl : public IWriteAwaitable {
 public:
  WriteAwaitableImpl(IoEvent* ioEvent, bool canWriting, const void* buf,
                     size_t count)
      : IWriteAwaitable(ioEvent, canWriting, buf, count) {}
  ~WriteAwaitableImpl() override = default;

  bool await_ready() noexcept override {
    if (!canWriting_) return true;
    retValue_ = write(ioEvent_->GetFd(), buf_, count_);
    retReady_ = retValue_ >= 0 || (retValue_ == -1 && errno != EAGAIN);
    return retReady_;
  }

  ssize_t await_resume() noexcept override {
    if (!canWriting_) {  // not connected
      errno = ESHUTDOWN;
      return -1;
    } else if (!retReady_) {
      retValue_ = write(ioEvent_->GetFd(), buf_, count_);
      ioEvent_->DisableWriting();
    }
    return retValue_;
  }

 private:
  bool retReady_ = false;
};

// FIXME implement it later
class SSLWriteAwaitableImpl;

class WriteAwaitable {
 public:
  WriteAwaitable(IoEvent* ioEvent, bool canWriting, const void* buf,
                 size_t count, bool ssl)
      : impl_(ssl ? std::make_unique<WriteAwaitableImpl>(ioEvent, canWriting,
                                                         buf, count)
                  : std::make_unique<WriteAwaitableImpl>(ioEvent, canWriting,
                                                         buf, count)) {}
  ~WriteAwaitable() = default;

  WriteAwaitable(const WriteAwaitable&) = delete;
  WriteAwaitable& operator=(const WriteAwaitable&) = delete;
  WriteAwaitable(WriteAwaitable&&) = default;
  WriteAwaitable& operator=(WriteAwaitable&&) = default;

  bool await_ready() noexcept { return impl_->await_ready(); }
  void await_suspend(std::coroutine_handle<> handle) noexcept {
    impl_->await_suspend(handle);
  }
  ssize_t await_resume() noexcept { return impl_->await_resume(); }

 private:
  std::unique_ptr<IWriteAwaitable> impl_;
};

class AcceptAwaitable : public IoAwaitableBaseForET {
 public:
  AcceptAwaitable(IoEvent* ioEvent, sockaddr* addr, socklen_t* addrlen)
      : IoAwaitableBaseForET(ioEvent, true), addr_(addr), addrlen_(addrlen) {}
  ~AcceptAwaitable() override = default;

  bool await_ready() noexcept {
    retValue_ = accept4(ioEvent_->GetFd(), addr_, addrlen_,
                        SOCK_NONBLOCK | SOCK_CLOEXEC);
    retReady_ = retValue_ >= 0 || (retValue_ == -1 && errno != EAGAIN);
    return retReady_;
  }

  int await_resume() noexcept {
    if (!retReady_) {
      retValue_ = accept4(ioEvent_->GetFd(), addr_, addrlen_,
                          SOCK_NONBLOCK | SOCK_CLOEXEC);
    }
    return retValue_;
  }

 private:
  sockaddr* addr_;
  socklen_t* addrlen_;
  int retValue_ = -1;
  bool retReady_ = false;
};

// FIXME implement it later
class HandleShakeAwaitable;

}  // namespace Cold::detail

#endif /* COLD_IO_IOAWAITABLE */
