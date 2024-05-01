#ifndef COLD_UTIL_SCOPEUTIL
#define COLD_UTIL_SCOPEUTIL

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

}  // namespace Cold::Base

#endif /* COLD_UTIL_SCOPEUTIL */
