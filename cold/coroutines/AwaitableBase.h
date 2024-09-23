#ifndef COLD_COROUTINES_AWAITABLEBASE
#define COLD_COROUTINES_AWAITABLEBASE

#include "Task.h"

namespace Cold {

class IoContext;

namespace detail {

class AwaitableBase {
 public:
  AwaitableBase(IoContext* context) noexcept : ioContext_(context) {}
  virtual ~AwaitableBase() noexcept = default;

  AwaitableBase(const AwaitableBase&) = delete;
  AwaitableBase& operator=(const AwaitableBase&) = delete;

  AwaitableBase(AwaitableBase&&) = default;
  AwaitableBase& operator=(AwaitableBase&&) = default;

  IoContext* GetIoContext() const { return ioContext_; }

 protected:
  IoContext* ioContext_ = nullptr;
};

template <typename T>
concept c_IsAwaitable = requires(T t) {
  {t.await_ready()};
  {t.await_suspend(std::coroutine_handle<>{})};
  {t.await_resume()};
};

template <typename T>
concept c_IsTask = requires(T t) {
  std::is_same_v<T, Task<typename T::value_type>>;
};

template <typename T>
struct IsAwaitable {
  static constexpr bool value = false;
  using value_type = typename T::value_type;
};

template <typename T>
requires c_IsAwaitable<T>
struct IsAwaitable<T> {
  static constexpr bool value = true;
  using value_type = decltype(std::declval<T>().await_resume());
};

template <typename T>
concept c_RequireBoth = c_IsAwaitable<T> || c_IsTask<T>;

}  // namespace detail

}  // namespace Cold

#endif /* COLD_COROUTINES_AWAITABLEBASE */
