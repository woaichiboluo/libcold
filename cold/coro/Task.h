#ifndef COLD_CORO_TASK
#define COLD_CORO_TASK

#include <cassert>
#include <coroutine>
#include <type_traits>
#include <utility>

namespace Cold::Base {

template <typename T = void>
class Task;

namespace Internal {

class PromiseBase {
 public:
  PromiseBase() noexcept = default;
  virtual ~PromiseBase() noexcept = default;

  auto initial_suspend() noexcept { return std::suspend_always(); }
  auto final_suspend() noexcept { return FinalAwaitable(); }
  void unhandled_exception() noexcept { assert(false); }

  void SetContinuation(std::coroutine_handle<> continuation) {
    continuation_ = continuation;
  }

 private:
  struct FinalAwaitable {
    bool await_ready() noexcept { return false; }

    template <typename T>
    std::coroutine_handle<> await_suspend(
        std::coroutine_handle<T> handle) noexcept {
      auto& promise = handle.promise();
      return promise.continuation_;
    }

    void await_resume() noexcept {}
  };

  std::coroutine_handle<> continuation_ = std::noop_coroutine();
};

template <typename T>
class TaskPromise : public PromiseBase {
 public:
  TaskPromise() noexcept = default;
  ~TaskPromise() noexcept = default;

  Task<T> get_return_object() noexcept;
  void return_value(T&& value) noexcept { value_ = std::move(value); }

  T& Result() { return value_; }

 private:
  T value_;
};

template <>
class TaskPromise<void> : public PromiseBase {
 public:
  TaskPromise() noexcept = default;
  ~TaskPromise() noexcept = default;

  Task<> get_return_object() noexcept;
  void return_void() noexcept {}
};

}  // namespace Internal

template <typename T>
class Task {
 public:
  using promise_type = Internal::TaskPromise<T>;
  using value_type = T;

  Task() noexcept = default;
  explicit Task(std::coroutine_handle<promise_type> handle) noexcept
      : handle_(handle) {}
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

  void Resume() const {
    if (handle_) handle_.resume();
  }

  auto Result() {
    assert(handle_ && handle_.done());
    return handle_.promise().Result();
  }

  std::coroutine_handle<> GetHandle() const { return handle_; }

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
      currentCoro_.promise().SetContinuation(caller);
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

namespace Internal {

template <typename T>
Task<T> TaskPromise<T>::get_return_object() noexcept {
  return Task<T>(std::coroutine_handle<TaskPromise>::from_promise(*this));
}

inline Task<> TaskPromise<void>::get_return_object() noexcept {
  return Task<>(std::coroutine_handle<TaskPromise>::from_promise(*this));
}

}  // namespace Internal

}  // namespace Cold::Base

#endif /* COLD_CORO_TASK */
