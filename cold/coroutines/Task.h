#ifndef COLD_COROUTINES_TASK
#define COLD_COROUTINES_TASK

#include <cassert>
#include <coroutine>
#include <utility>

namespace Cold {

class Scheduler;
class IoContext;

template <typename T = void>
class Task;

namespace detail {
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

  bool IsReady() const { return ready_; }

  void SetReady() { ready_ = true; }

  void SetScheduler(Scheduler* scheduler) { scheduler_ = scheduler; }

 private:
  struct FinalAwaitable {
    bool await_ready() noexcept { return false; }

    template <typename T>
    void await_suspend(std::coroutine_handle<T> handle) noexcept;

    void await_resume() noexcept {}
  };

  bool ready_ = false;
  std::coroutine_handle<> continuation_;
  Scheduler* scheduler_ = nullptr;
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

}  // namespace detail

template <typename T>
class Task {
 public:
  friend class Scheduler;
  friend class IoContext;

  using promise_type = detail::TaskPromise<T>;
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

    bool await_suspend(std::coroutine_handle<> caller) noexcept {
      currentCoro_.promise().SetContinuation(caller);
      currentCoro_.resume();
      auto ready = currentCoro_.promise().IsReady();
      currentCoro_.promise().SetReady();
      return !ready;
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

namespace detail {

template <typename T>
Task<T> TaskPromise<T>::get_return_object() noexcept {
  return Task<T>(std::coroutine_handle<TaskPromise>::from_promise(*this));
}

inline Task<> TaskPromise<void>::get_return_object() noexcept {
  return Task<>(std::coroutine_handle<TaskPromise>::from_promise(*this));
}

}  // namespace detail

}  // namespace Cold

#endif /* COLD_COROUTINES_TASK */
