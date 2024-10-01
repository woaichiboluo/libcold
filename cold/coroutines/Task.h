#ifndef COLD_COROUTINES_TASK
#define COLD_COROUTINES_TASK

#include "../detail/TaskBase.h"

namespace Cold {

template <typename T>
class Task {
 public:
  friend class IoContext;

  using promise_type = Detail::TaskPromise<T>;
  using value_type = T;

  Task() noexcept = default;
  explicit Task(std::coroutine_handle<promise_type> h) noexcept : handle_(h) {}
  ~Task() {
    if (handle_) handle_.destroy();
  }

  Task(const Task&) = delete;
  Task& operator=(const Task&) = delete;

  Task(Task&& other) noexcept : handle_(other.handle_) {
    other.handle_ = nullptr;
  }

  Task& operator=(Task&& other) noexcept {
    if (&other == this) return *this;
    if (handle_) handle_.destroy();
    handle_ = other.handle_;
    other.handle_ = nullptr;
    return *this;
  }

  bool Done() { return !handle_ || handle_.done(); }

  std::coroutine_handle<> Release() {
    assert(handle_);
    auto handle = handle_;
    handle_ = nullptr;
    return handle;
  }

  void SetIoContext(IoContext& context) {
    assert(handle_);
    handle_.promise().SetIoContext(context);
  }

  IoContext& GetIoContext() {
    assert(handle_);
    return handle_.promise().GetIoContext();
  }

  auto Result() {
    assert(handle_ && handle_.done());
    return handle_.promise().Result();
  }

  std::coroutine_handle<promise_type> GetHandle() const { return handle_; }

  auto operator co_await() const noexcept {
    assert(handle_);
    return TaskAwaitable(handle_);
  }

 private:
  struct TaskAwaitable {
    TaskAwaitable(std::coroutine_handle<promise_type> handle)
        : currentCoro_(handle) {}
    ~TaskAwaitable() = default;

    bool await_ready() noexcept { return false; }

    std::coroutine_handle<> await_suspend(
        std::coroutine_handle<> caller) noexcept {
      auto& callerPromise =
          std::coroutine_handle<Detail::PromiseBase>::from_address(
              caller.address())
              .promise();
      auto& currentCoroPromise = currentCoro_.promise();
      currentCoroPromise.SetIoContext(callerPromise.GetIoContext());
      currentCoroPromise.SetContinuation(caller);
      currentCoroPromise.SetStopToken(callerPromise.GetStopToken());
      return currentCoro_;
    }

    auto await_resume() noexcept {
      if constexpr (std::is_same_v<void, T>) {
        return;
      } else {
        return std::move(currentCoro_.promise().Result());
      }
    }

    std::coroutine_handle<promise_type> currentCoro_;
  };

  std::coroutine_handle<promise_type> handle_;
};

namespace Detail {

template <typename T>
Task<T> TaskPromise<T>::get_return_object() noexcept {
  return Task<T>(std::coroutine_handle<TaskPromise>::from_promise(*this));
}

inline Task<> TaskPromise<void>::get_return_object() noexcept {
  return Task<>(std::coroutine_handle<TaskPromise>::from_promise(*this));
}

}  // namespace Detail

}  // namespace Cold

#endif /* COLD_COROUTINES_TASK */
