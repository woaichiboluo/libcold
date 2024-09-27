#ifndef COLD_DETAIL_TIMEOUTAWAITABLE
#define COLD_DETAIL_TIMEOUTAWAITABLE

#include "../coroutines/ThisCoro.h"
#include "../io/IoContext.h"

namespace Cold::Detail {

template <typename T, typename REP, typename PERIOD>
class TimeoutAwaitable {
 public:
  TimeoutAwaitable(IoContext& context, Task<T> task,
                   std::chrono::duration<REP, PERIOD> duration)
      : timer_(context), task_(std::move(task)), duration_(duration) {}
  ~TimeoutAwaitable() = default;

  struct WhenVoidType {};

  using value_type =
      std::conditional_t<std::is_same_v<T, void>, WhenVoidType, T>;

  bool await_ready() noexcept { return false; }

  void await_suspend(std::coroutine_handle<> handle) noexcept {
    auto wrapTask = [](Task<T> task, std::stop_token token, value_type& ret,
                       std::coroutine_handle<> h) -> Task<> {
      value_type v;
      co_await ThisCoro::SetStopToken(token);
      if constexpr (std::is_same_v<value_type, WhenVoidType>) {
        co_await task;
      } else {
        v = co_await task;
      }
      if (token.stop_requested()) co_return;
      ret = std::move(v);
      h.resume();
    }(std::move(task_), source_.get_token(), retValue_, handle);
    timer_.GetIoContext().CoSpawn(std::move(wrapTask));
    timer_.ExpiresAfter(duration_);
    timer_.AsyncWait([s = source_, handle]() {
      s.request_stop();
      handle.resume();
    });
  }

  auto await_resume() noexcept {
    bool timeout = source_.stop_requested();
    if (!timeout) timer_.Cancel();
    if constexpr (std::is_same_v<value_type, WhenVoidType>) {
      return timeout;
    } else {
      return std::pair<bool, value_type>(timeout, std::move(retValue_));
    }
  }

 private:
  Timer timer_;
  Task<T> task_;
  std::chrono::duration<REP, PERIOD> duration_;
  std::stop_source source_;
  value_type retValue_;
};

}  // namespace Cold::Detail

#endif /* COLD_DETAIL_TIMEOUTAWAITABLE */
