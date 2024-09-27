#ifndef COLD_DETAIL_IOAWAITABLE
#define COLD_DETAIL_IOAWAITABLE

#include <memory>

#include "IoEvent.h"

namespace Cold::Detail {

class IoAwaitable {
 public:
  static constexpr uint32_t kRead = 1;
  static constexpr uint32_t kWrite = 1 << 1;
  static constexpr uint32_t kEnable = 1 << 2;

  IoAwaitable(std::weak_ptr<IoEvent> event, uint32_t ioType)
      : event_(std::move(event)), ioType_(ioType) {}

  ~IoAwaitable() { Clean(); }

  bool await_ready() const { return false; }

  void await_suspend(std::coroutine_handle<> handle) {
    auto event = event_.lock();
    if (!event) return;
    if (ioType_ & kRead) {
      event->SetOnReadCoroutine(handle);
      if (ioType_ & kEnable) event->EnableReading();
    } else if (ioType_ & kWrite) {
      event->SetOnWriteCoroutine(handle);
      if (ioType_ & kEnable) event->EnableWriting();
    }
    isPendingIo_ = true;
  }

  void await_resume() {
    Clean();
    isPendingIo_ = false;
  }

 private:
  void Clean() {
    if (!isPendingIo_) return;
    auto event = event_.lock();
    if (!event) return;
    if (ioType_ & kRead) {
      event->ClearReadCoroutine();
      if (ioType_ & kEnable) event->DisableReading();
    } else if (ioType_ & kWrite) {
      event->ClearWriteCoroutine();
      if (ioType_ & kEnable) event->DisableWriting();
    }
  }

  std::weak_ptr<IoEvent> event_;
  uint32_t ioType_;
  bool isPendingIo_ = false;
};

inline IoAwaitable ReadIoAwaitable(std::weak_ptr<IoEvent> event,
                                   bool enableRead) {
  uint32_t ioType =
      IoAwaitable::kRead | (enableRead ? IoAwaitable::kEnable : 0);
  return IoAwaitable(event, ioType);
}

inline IoAwaitable WriteIoAwaitable(std::weak_ptr<IoEvent> event,
                                    bool enableWrite) {
  uint32_t ioType =
      IoAwaitable::kWrite | (enableWrite ? IoAwaitable::kEnable : 0);
  return IoAwaitable(event, ioType);
}

}  // namespace Cold::Detail

#endif /* COLD_DETAIL_IOAWAITABLE */
