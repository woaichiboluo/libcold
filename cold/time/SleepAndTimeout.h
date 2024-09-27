#ifndef COLD_TIME_SLEEPANDTIMEOUT
#define COLD_TIME_SLEEPANDTIMEOUT

#include "../coroutines/ThisCoro.h"
#include "../detail/TimeoutAwaitable.h"
#include "Timer.h"

namespace Cold {

template <typename T, typename REP, typename PERIOD>
[[nodiscard]] auto Timeout(Task<T> task,
                           std::chrono::duration<REP, PERIOD> duration)
    -> Task<
        std::conditional_t<std::is_same_v<T, void>, bool, std::pair<bool, T>>> {
  auto& context = co_await ThisCoro::GetIoContext{};
  co_return co_await Detail::TimeoutAwaitable(context, std::move(task),
                                              duration);
}

template <typename REP, typename PERIOD>
[[nodiscard]] Task<> Sleep(std::chrono::duration<REP, PERIOD> duration) {
  auto& context = co_await ThisCoro::GetIoContext{};
  Timer timer(context);
  timer.ExpiresAfter(duration);
  co_await timer.AsyncWaitable([]() -> Task<> { co_return; }());
}

}  // namespace Cold

#endif /* COLD_TIME_SLEEPANDTIMEOUT */
