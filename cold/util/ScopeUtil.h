#ifndef COLD_UTIL_SCOPEUTIL
#define COLD_UTIL_SCOPEUTIL

#include <utility>

namespace Cold {

template <typename T>
class ScopeGuard {
 public:
  ScopeGuard(T&& f) : call_(std::move(f)) {}

  ~ScopeGuard() { call_(); }

  ScopeGuard(const ScopeGuard&) = delete;
  ScopeGuard& operator=(const ScopeGuard&) = delete;

 private:
  T call_;
};

}  // namespace Cold

#endif /* COLD_UTIL_SCOPEUTIL */
