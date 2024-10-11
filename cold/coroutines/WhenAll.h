#ifndef COLD_COROUTINES_WHENALL
#define COLD_COROUTINES_WHENALL

#include "../detail/VoidOrDecay.h"
#include "CountDownLatch.h"
#include "ThisCoro.h"

namespace Cold {

template <typename... TaskType>
using AllResult = std::tuple<Detail::VoidOrDecay<TaskType>...>;

template <typename T>
Task<> WhenAllTask(Task<T> task, T& value, CountDownLatch& latch) {
  if constexpr (std::is_void_v<T>) {
    co_await task;
  } else {
    value = co_await task;
  }
  co_await latch.CountDown();
}

template <size_t... N, typename... TaskTypes>
Task<AllResult<TaskTypes...>> WhenAllImpl(std::index_sequence<N...>,
                                          Task<TaskTypes>... tasks) {
  auto& ioContext = co_await ThisCoro::GetIoContext();
  AllResult<TaskTypes...> result;
  CountDownLatch latch(sizeof...(tasks));
  (ioContext.CoSpawn(WhenAllTask(std::move(tasks), std::get<N>(result), latch)),
   ...);
  co_await latch.Wait();
  co_return std::move(result);
}

template <typename... TaskTypes>
[[nodiscard]] Task<AllResult<TaskTypes...>> WhenAll(Task<TaskTypes>... tasks) {
  return WhenAllImpl(std::make_index_sequence<sizeof...(tasks)>(),
                     std::move(tasks)...);
}

}  // namespace Cold

#endif /* COLD_COROUTINES_WHENALL */
