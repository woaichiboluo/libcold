#ifndef COLD_TIME_TIMEAWAITABLE
#define COLD_TIME_TIMEAWAITABLE

#include <chrono>

#include "Timer.h"

namespace Cold {

namespace detail {

template <typename T, typename REP, typename PERIOOD>
requires c_RequireBoth<T>
class Timeout : public detail::AwaitableBase {
 public:
  struct WhenVoidType {};

  using value_type = typename detail::IsAwaitable<T>::value_type;

  using ValueType = std::conditional_t<std::is_same_v<void, value_type>,
                                       WhenVoidType, value_type>;

  Timeout(IoContext& context, T&& t,
          std::chrono::duration<REP, PERIOOD> duration) noexcept
      : detail::AwaitableBase(&context),
        timeoutContext_(std::make_shared<TimeoutContext>()),
        duration_(duration),
        timer_(*ioContext_) {
    timeoutContext_->job = std::move(t);
  }

  ~Timeout() override = default;

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
      if constexpr (HasTimeout<T>::value) {
        timeoutContext_->job.OnTimeout();
      }
      timeoutContext_->timeout = true;
      handle.resume();
    });
    ioContext_->CoSpawn(std::move(wrapTask));
  }

  std::pair<bool, ValueType> await_resume() noexcept
      requires(!std::is_same_v<ValueType, WhenVoidType>) {
    if (!timeoutContext_->timeout) {
      timer_.Cancel();
    } else {
      ioContext_->TaskDone(timeoutContext_->handle);
    }
    return {timeoutContext_->timeout, timeoutContext_->retValue};
  }

  bool await_resume() noexcept
      requires(std::is_same_v<ValueType, WhenVoidType>) {
    if (!timeoutContext_->timeout) {
      timer_.Cancel();
    } else {
      ioContext_->TaskDone(timeoutContext_->handle);
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

  std::shared_ptr<TimeoutContext> timeoutContext_;
  std::chrono::duration<REP, PERIOOD> duration_;
  Timer timer_;
};

}  // namespace detail

template <typename REP, typename PERIOD>
class Sleep : public detail::AwaitableBase {
 public:
  Sleep(IoContext& context, std::chrono::duration<REP, PERIOD> duration)
      : detail::AwaitableBase(&context), duration_(duration), timer_(context) {}

  ~Sleep() override = default;

  bool await_ready() noexcept { return false; }

  void await_suspend(std::coroutine_handle<> handle) noexcept {
    timer_.ExpiresAfter(duration_);
    timer_.AsyncWait([handle]() { handle.resume(); });
  }

  void await_resume() noexcept {}

 private:
  std::chrono::duration<REP, PERIOD> duration_;
  Timer timer_;
};

}  // namespace Cold

#endif /* COLD_TIME_TIMEAWAITABLE */
