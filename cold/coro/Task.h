#ifndef COLD_CORO_TASK
#define COLD_CORO_TASK

#include <atomic>
#include <coroutine>
#include <memory>
#include <type_traits>
#include <utility>

// Task.h modified from cppcoro include/cppcoro/task.hpp
// https://github.com/lewissbaker/cppcoro/blob/master/include/cppcoro/task.hpp

namespace Cold::Base {
template <typename T>
class Task;

namespace internal {
class PromiseBase {
 public:
  PromiseBase() noexcept = default;
  virtual ~PromiseBase() noexcept = default;

  auto initial_suspend() noexcept { return std::suspend_always{}; }
  auto final_suspend() noexcept { return FinalAwaitable{}; }
  void unhandled_exception() noexcept {}

  void SetContinuation(std::coroutine_handle<> continuation) {
    continuation_ = continuation;
  }

 private:
  struct FinalAwaitable {
    bool await_ready() noexcept { return false; }
    template <typename PROMISE>
    void await_suspend(std::coroutine_handle<PROMISE> handle) noexcept {
      auto& promise = handle.promise();
      if (promise.continuation_) promise.continuation_.resume();
    }
    void await_resume() noexcept {}
  };

  std::coroutine_handle<> continuation_;
};

template <typename T>
class Promise : public PromiseBase {
 public:
  Promise() = default;
  ~Promise() noexcept override = default;

  Task<T> get_return_object() noexcept;

  void return_value(T&& value) noexcept { returnValue_ = std::move(value); }

  T& Result() & { return returnValue_; }

 private:
  T returnValue_;
};

template <>
class Promise<void> : public PromiseBase {
 public:
  Promise() noexcept = default;
  ~Promise() noexcept override = default;

  Task<void> get_return_object() noexcept;
  void return_void() noexcept {}
};

}  // namespace internal

template <typename T = void>
class Task {
 public:
  using promise_type = internal::Promise<T>;
  using value_type = T;

  Task() = default;
  explicit Task(std::coroutine_handle<promise_type> handle) : handle_(handle) {}
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

  bool Done() const { return !handle_ || handle_.done(); }

  void Resume() const {
    if (handle_) handle_.resume();
  }

  std::coroutine_handle<> GetHandle() { return handle_; }

  auto operator co_await() const noexcept { return Awaitable(handle_); }

 private:
  struct Awaitable {
    Awaitable(std::coroutine_handle<promise_type> handle) : coroutine(handle) {}
    ~Awaitable() = default;

    bool await_ready() noexcept { return false; }

    void await_suspend(std::coroutine_handle<> handle) noexcept {
      coroutine.promise().SetContinuation(handle);
      coroutine.resume();
    }
    auto await_resume() noexcept {
      if constexpr (std::is_same_v<value_type, void>)
        return;
      else
        return std::move(coroutine.promise().Result());
    }

    std::coroutine_handle<promise_type> coroutine;
  };

  std::coroutine_handle<promise_type> handle_;
};

namespace internal {
template <typename T>
Task<T> Promise<T>::get_return_object() noexcept {
  return Task<T>{std::coroutine_handle<Promise>::from_promise(*this)};
}

inline Task<void> Promise<void>::get_return_object() noexcept {
  return Task<void>{std::coroutine_handle<Promise>::from_promise(*this)};
}

}  // namespace internal

}  // namespace Cold::Base

#endif /* COLD_CORE_TASK */
