#ifndef COLD_UTIL_SCOPEUTIL
#define COLD_UTIL_SCOPEUTIL

#include <unistd.h>

#include <functional>
#include <utility>

namespace Cold::Base {

template <typename Callable>
class ScopeGuard {
 public:
  ScopeGuard(Callable&& call) : call_(std::move(call)) {}

  ~ScopeGuard() { call_(); }

  ScopeGuard(const ScopeGuard&) = delete;
  ScopeGuard& operator=(const ScopeGuard&) = delete;

 private:
  Callable call_;
};

class ScopeFdGuard {
 public:
  ScopeFdGuard(int fd) noexcept : fd_(fd) {}
  ~ScopeFdGuard() noexcept {
    if (fd_ >= 0) close(fd_);
  }

  ScopeFdGuard(const ScopeFdGuard&) = delete;
  ScopeFdGuard& operator=(const ScopeFdGuard&) = delete;

  operator int() { return fd_; }

 private:
  int fd_;
};

}  // namespace Cold::Base

#endif /* COLD_UTIL_SCOPEUTIL */
