#ifndef COLD_CORO_IOWATCHER_INI
#define COLD_CORO_IOWATCHER_INI

#include <memory>

#include "IoWatcher.h"
#include "IoWatcherEpoll.h"

namespace Cold::Base {

namespace internal {
inline std::unique_ptr<IoWatcher> GetDefaultIoWatcher() {
  return std::make_unique<IoWatcherEpoll>();
}
}  // namespace internal

}  // namespace Cold::Base

#endif /* COLD_CORO_IOWATCHER_INI */
