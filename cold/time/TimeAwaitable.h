#ifndef COLD_TIME_TIMEAWAITABLE
#define COLD_TIME_TIMEAWAITABLE

#include <chrono>

#include "Timer.h"

namespace Cold {

namespace detail {

template <typename T>
concept CIsAwaitable = requires(T t) {
  {t.await_ready()};
  {t.await_suspend(std::coroutine_handle<>{})};
  {t.await_resume()};
};

template <typename T>
struct IsAwaitable {
  static constexpr bool value = false;
  using value_type = typename T::value_type;
};

template <typename T>
requires CIsAwaitable<T>
struct IsAwaitable<T> {
  static constexpr bool value = true;
  using value_type = decltype(std::declval<T>().await_resume());
};

}  // namespace detail

template <typename T, typename REP, typename PERIOOD>
requires(detail::IsAwaitable<T>::value ||
         std::is_same_v<T, Task<typename T::value_type>>) class Timeout {
 public:
  struct WhenVoidType {};

  using value_type = typename detail::IsAwaitable<T>::value_type;

  using ValueType = std::conditional_t<std::is_same_v<void, value_type>,
                                       WhenVoidType, value_type>;

  Timeout(IoContext& context, T&& t,
          std::chrono::duration<REP, PERIOOD> duration) noexcept
      : context_(&context),
        timeoutContext_(std::make_shared<TimeoutContext>()),
        duration_(duration),
        timer_(*context_) {
    timeoutContext_->job = std::move(t);
  }
  ~Timeout() = default;

  Timeout(const Timeout&) = delete;
  Timeout& operator=(const Timeout&) = delete;
  Timeout(Timeout&&) = default;
  Timeout& operator=(Timeout&&) = default;

  bool await_ready() noexcept { return false; }

  void await_suspend(std::coroutine_handle<> handle) noexcept {
    auto wrapTask = [](std::shared_ptr<TimeoutContext> context,
                       std::coroutine_handle<> h) -> Task<> {
      if constexpr (std::is_same_v<ValueType, WhenVoidType>) {
        co_await context->job;
      } else {
        context->retValue = co_await context->job;
      }
      if (!context->timeout) h.resume();
    }(timeoutContext_, handle);
    timeoutContext_->handle = wrapTask.GetHandle();
    timer_.ExpiresAfter(duration_);
    timer_.AsyncWait([&, handle]() {
      if constexpr (requires { timeoutContext_->job.OnTimeout(); }) {
        timeoutContext_->job.OnTimeout();
      }
      timeoutContext_->timeout = true;
      handle.resume();
    });
    context_->CoSpawn(std::move(wrapTask));
  }

  std::pair<bool, ValueType> await_resume() noexcept
      requires(!std::is_same_v<ValueType, WhenVoidType>) {
    if (!timeoutContext_->timeout) {
      timer_.Cancel();
    } else {
      context_->TaskDone(timeoutContext_->handle);
    }
    return {timeoutContext_->timeout, timeoutContext_->retValue};
  }

  bool await_resume() noexcept
      requires(std::is_same_v<ValueType, WhenVoidType>) {
    if (!timeoutContext_->timeout) {
      timer_.Cancel();
    } else {
      context_->TaskDone(timeoutContext_->handle);
    }
    return timeoutContext_->timeout;
  }

 private:
  struct TimeoutContext {
    T job;
    ValueType retValue{};
    bool timeout = false;
    std::coroutine_handle<> handle;
  };

  IoContext* context_;
  std::shared_ptr<TimeoutContext> timeoutContext_;
  std::chrono::duration<REP, PERIOOD> duration_;
  Timer timer_;
};

template <typename REP, typename PERIOD>
class Sleep {
 public:
  Sleep(IoContext& context, std::chrono::duration<REP, PERIOD> duration)
      : context_(&context), duration_(duration), timer_(context) {}

  ~Sleep() = default;

  Sleep(const Sleep&) = delete;
  Sleep& operator=(const Sleep&) = delete;

  Sleep(Sleep&&) = default;
  Sleep& operator=(Sleep&&) = default;

  bool await_ready() noexcept { return false; }

  void await_suspend(std::coroutine_handle<> handle) noexcept {
    timer_.ExpiresAfter(duration_);
    timer_.AsyncWait([handle]() { handle.resume(); });
  }

  void await_resume() noexcept {}

 private:
  IoContext* context_;
  std::chrono::duration<REP, PERIOD> duration_;
  Timer timer_;
};

}  // namespace Cold

#endif /* COLD_TIME_TIMEAWAITABLE */
