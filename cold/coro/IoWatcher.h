#ifndef COLD_CORO_IOWATCHER
#define COLD_CORO_IOWATCHER

#include <coroutine>
#include <cstdint>
#include <vector>

namespace Cold::Base {

namespace internal {

enum class Mode : uint32_t {
  READ = 1 << 0,
  WRITE = 1 << 2,
  DISABLE_ALL = 0,
  DISABLE_READ = ~READ,
  DISABLE_WRITE = ~WRITE
};

struct IoEvent {
  int fd;
  Mode mode;
  std::coroutine_handle<> callbackCoroutine = nullptr;
};

}  // namespace internal

class IoWatcher {
 public:
  IoWatcher() = default;
  virtual ~IoWatcher() = default;

  IoWatcher(const IoWatcher&) = delete;
  IoWatcher& operator=(const IoWatcher&) = delete;

  virtual const std::vector<std::coroutine_handle<>>& WatchIo(int waitTime) = 0;
  virtual void HandleIoEvent(const internal::IoEvent& event) = 0;
  virtual void Wakeup() = 0;
};

};  // namespace Cold::Base

#endif /* COLD_CORO_IOWATCHER */
