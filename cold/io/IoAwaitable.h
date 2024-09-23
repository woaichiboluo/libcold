#ifndef COLD_IO_IOAWAITABLE
#define COLD_IO_IOAWAITABLE

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <memory>

#include "../coroutines/AwaitableBase.h"
#include "IoEvent.h"

namespace Cold::detail {

// for non-blocking io
// mainly for the socket
class IoAwaitableBaseForET : public AwaitableBase {
 public:
  IoAwaitableBaseForET(IoEvent* ioEvent, bool ioForRead) noexcept
      : AwaitableBase(&ioEvent->GetIoContext()),
        ioEvent_(ioEvent),
        ioForRead_(ioForRead) {}

  virtual ~IoAwaitableBaseForET() {
    if (isPendingIO_) OnTimeout();
  }

  void await_suspend(std::coroutine_handle<> handle) noexcept {
    if (ioForRead_) {
      ioEvent_->SetOnReadCoroutine(handle);
    } else {
      ioEvent_->SetOnWriteCoroutine(handle);
      ioEvent_->EnableWritingET();
    }
    isPendingIO_ = true;
  }

  void OnTimeout() noexcept {
    if (ioForRead_) {
      ioEvent_->ClearReadCoroutine();
    } else {
      ioEvent_->DisableWriting();
    }
    isPendingIO_ = false;
  }

 protected:
  IoEvent* ioEvent_;
  bool ioForRead_;
  bool isPendingIO_ = false;
};

// for blocking non-blocking io
// so when await_ready,always watch the io event
// when solve the file and stdio, we use the blocking io,so use LT mode
class IoAwaitableBaseForLT : public AwaitableBase {
 public:
  IoAwaitableBaseForLT(IoEvent* ioEvent, bool read)
      : AwaitableBase(&ioEvent->GetIoContext()),
        ioEvent_(ioEvent),
        ioForRead_(read) {}
  virtual ~IoAwaitableBaseForLT() {
    if (isPendingIO_) OnTimeout();
  }

  bool await_ready() noexcept { return false; }

  void await_suspend(std::coroutine_handle<> handle) noexcept {
    if (ioForRead_) {
      ioEvent_->SetOnReadCoroutine(handle);
      ioEvent_->EnableReading();
    } else {
      ioEvent_->SetOnWriteCoroutine(handle);
      ioEvent_->EnableWriting();
    }
    isPendingIO_ = true;
  }

  void OnTimeout() noexcept {
    if (ioForRead_) {
      ioEvent_->DisableReading();
    } else {
      ioEvent_->DisableWriting();
    }
    isPendingIO_ = false;
  }

 protected:
  IoEvent* ioEvent_;
  bool ioForRead_;
  bool isPendingIO_ = false;
};

class ReadAwaitableForLT : public IoAwaitableBaseForLT {
 public:
  ReadAwaitableForLT(IoEvent* ioEvent, void* buf, size_t count)
      : IoAwaitableBaseForLT(ioEvent, true), buf_(buf), count_(count) {}
  ~ReadAwaitableForLT() override = default;

  ssize_t await_resume() noexcept {
    isPendingIO_ = false;
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
    isPendingIO_ = false;
    return write(ioEvent_->GetFd(), buf_, count_);
  }

 private:
  const void* buf_;
  size_t count_;
};

class ReadAwaitable : public IoAwaitableBaseForET {
 public:
  ReadAwaitable(IoEvent* ioEvent, bool canReading, void* buf, size_t count)
      : IoAwaitableBaseForET(ioEvent, true),
        canReading_(canReading),
        buf_(buf),
        count_(count) {}
  ~ReadAwaitable() override = default;

  bool await_ready() noexcept {
    if (!canReading_) return true;
    retValue_ = read(ioEvent_->GetFd(), buf_, count_);
    retReady_ = retValue_ >= 0 || (retValue_ == -1 && errno != EAGAIN);
    return retReady_;
  }

  ssize_t await_resume() noexcept {
    isPendingIO_ = false;
    if (!canReading_) {  // not connected
      errno = ESHUTDOWN;
      return -1;
    } else if (!retReady_) {
      retValue_ = read(ioEvent_->GetFd(), buf_, count_);
    }
    return retValue_;
  }

 private:
  bool canReading_;
  void* buf_;
  size_t count_;
  ssize_t retValue_ = -1;
  bool retReady_ = false;
};

class WriteAwaitable : public IoAwaitableBaseForET {
 public:
  WriteAwaitable(IoEvent* ioEvent, bool canWriting, const void* buf,
                 size_t count)
      : IoAwaitableBaseForET(ioEvent, false) {}
  ~WriteAwaitable() override = default;

  bool await_ready() noexcept {
    if (!canWriting_) return true;
    retValue_ = write(ioEvent_->GetFd(), buf_, count_);
    retReady_ = retValue_ >= 0 || (retValue_ == -1 && errno != EAGAIN);
    return retReady_;
  }

  ssize_t await_resume() noexcept {
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
  bool canWriting_;
  const void* buf_;
  size_t count_;
  ssize_t retValue_ = -1;
  bool retReady_ = false;
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

  sockaddr* addr_;
  socklen_t* addrlen_;
  int retValue_ = -1;
  bool retReady_ = false;
};

class ConnectAwaitable : public IoAwaitableBaseForET {
 public:
  ConnectAwaitable(IoEvent* ioEvent, const sockaddr* addr, socklen_t addrlen)
      : IoAwaitableBaseForET(ioEvent, false), addr_(addr), addrlen_(addrlen) {}
  ~ConnectAwaitable() override = default;

  bool await_ready() noexcept {
    retValue_ = connect(ioEvent_->GetFd(), addr_, addrlen_);
    inProgress_ = retValue_ == -1 && errno == EINPROGRESS;
    retReady_ = retValue_ == 0 || (retValue_ == -1 && errno != EINPROGRESS);
    return retReady_;
  }

  int await_resume() noexcept {
    if (inProgress_) {
      assert(retValue_ == -1);
      socklen_t len = sizeof(int);
      if (getsockopt(ioEvent_->GetFd(), SOL_SOCKET, SO_ERROR, &retValue_,
                     &len) == 0) {
        errno = retValue_;
      }
    }
    return retValue_;
  }

 private:
  const sockaddr* addr_;
  socklen_t addrlen_;
  int retValue_ = -1;
  bool retReady_ = false;
  bool inProgress_ = false;
};

// FIME implement it later
class HandleShakeAwaitable;
class SSLWriteAwaitable;
class SSLReadAwaitableImpl;

}  // namespace Cold::detail

#endif /* COLD_IO_IOAWAITABLE */
