#ifndef COLD_DETAIL_TASKBASE
#define COLD_DETAIL_TASKBASE

#include <cassert>
#include <coroutine>
#include <stop_token>
#include <utility>

namespace Cold {

class IoContext;

template <typename T = void>
class Task;

namespace Detail {
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

  void SetIoContext(IoContext* ioContext) { ioContext_ = ioContext; }
  IoContext* GetIoContext() const { return ioContext_; }
  void SetCurExecuteIoContext(IoContext* executeContext) {
    executeContext_ = executeContext;
  }
  IoContext* GetCurExecuteIoContext() const { return executeContext_; }

  void SetStopToken(std::stop_token stopToken) { stopToken_ = stopToken; }
  std::stop_token GetStopToken() const { return stopToken_; }

 private:
  struct FinalAwaitable {
    bool await_ready() noexcept { return false; }

    template <typename T>
    std::coroutine_handle<> await_suspend(
        std::coroutine_handle<T> handle) noexcept {
      auto& promise = handle.promise();
      if (promise.continuation_) {
        return promise.continuation_;
      } else {
        return std::noop_coroutine();
      }
    }

    void await_resume() noexcept {}
  };

  std::coroutine_handle<> continuation_;
  // task create context
  IoContext* ioContext_ = nullptr;
  // current execute task context
  IoContext* executeContext_ = nullptr;
  std::stop_token stopToken_;
};

template <typename T>
class TaskPromise : public PromiseBase {
 public:
  TaskPromise() noexcept = default;
  ~TaskPromise() noexcept = default;

  Task<T> get_return_object() noexcept;
  void return_value(T value) noexcept { value_ = std::move(value); }

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

}  // namespace Detail
}  // namespace Cold

#endif /* COLD_DETAIL_TASKBASE */
