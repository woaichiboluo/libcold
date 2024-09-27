#ifndef COLD_COROUTINES_THISCORO
#define COLD_COROUTINES_THISCORO

#include <coroutine>
#include <stop_token>

namespace Cold {
class IoContext;
}

namespace Cold::ThisCoro {

struct GetIoContext {
  GetIoContext() noexcept = default;
  ~GetIoContext() noexcept = default;

  bool await_ready() const noexcept { return false; }

  template <typename T>
  bool await_suspend(std::coroutine_handle<T> handle) noexcept {
    context_ = handle.promise().GetIoContext();
    return false;
  }

  IoContext& await_resume() noexcept { return *context_; }

 private:
  IoContext* context_;
};

struct GetHandle {
  GetHandle() noexcept = default;
  ~GetHandle() noexcept = default;

  bool await_ready() const noexcept { return false; }

  bool await_suspend(std::coroutine_handle<> handle) noexcept {
    handle_ = handle;
    return false;
  }

  std::coroutine_handle<> await_resume() noexcept { return handle_; }

 private:
  std::coroutine_handle<> handle_;
};

struct GetStopToken {
  GetStopToken() noexcept = default;
  ~GetStopToken() noexcept = default;

  bool await_ready() const noexcept { return false; }

  template <typename T>
  bool await_suspend(std::coroutine_handle<T> handle) noexcept {
    stopToken_ = handle.promise().GetStopToken();
    return false;
  }

  std::stop_token await_resume() noexcept { return stopToken_; }

 private:
  std::stop_token stopToken_;
};

struct SetStopToken {
  SetStopToken(std::stop_token token) noexcept : stopToken_(std::move(token)) {}
  ~SetStopToken() noexcept = default;

  bool await_ready() const noexcept { return false; }

  template <typename T>
  bool await_suspend(std::coroutine_handle<T> handle) noexcept {
    handle.promise().SetStopToken(std::move(stopToken_));
    return false;
  }

  void await_resume() noexcept {}

  std::stop_token stopToken_;
};

}  // namespace Cold::ThisCoro

#endif /* COLD_COROUTINES_THISCORO */
