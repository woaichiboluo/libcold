#ifndef COLD_COROUTINES_WHENANY
#define COLD_COROUTINES_WHENANY

#include <memory>
#include <variant>

#include "../detail/VoidOrDecay.h"
#include "Task.h"
#include "ThisCoro.h"

namespace Cold {

template <typename... TaskType>
using AnyVariable = std::variant<Detail::VoidOrDecay<TaskType>...>;

template <typename... TaskType>
struct AnyTaskContext {
  std::atomic<size_t> arrive{0};
  std::stop_source source;
  AnyVariable<TaskType...> value;
  std::coroutine_handle<> continuation;
};

template <size_t index, typename T, typename... TaskType>
Task<> WhenAnyTask(Task<T> task,
                   std::shared_ptr<AnyTaskContext<TaskType...>> anyContext) {
  assert(!anyContext->source.stop_requested());
  assert(anyContext->arrive == 0);
  T v;
  if constexpr (std::is_void_v<T>) {
    co_await task;
  } else {
    v = co_await task;
  }
  if (anyContext->source.stop_requested()) {
    co_return;
  }

  anyContext->value =
      AnyVariable<TaskType...>(std::in_place_index_t<index>(), std::move(v));
  anyContext->arrive = index;
  anyContext->source.request_stop();
  anyContext->continuation.resume();
}

template <typename... TaskTypes>
using AnyResult = AnyVariable<TaskTypes...>;

template <size_t... N, typename... TaskTypes>
Task<AnyResult<TaskTypes...>> WhenAnyImpl(std::index_sequence<N...>,
                                          Task<TaskTypes>... tasks) {
  auto& ioContext = co_await ThisCoro::GetIoContext();
  auto handle = co_await ThisCoro::GetHandle();
  using AnyContextType = AnyTaskContext<TaskTypes...>;
  auto anyContext = std::make_shared<AnyContextType>();
  anyContext->continuation = handle;
  (ioContext.CoSpawn(WhenAnyTask<N>(std::move(tasks), anyContext) |
                     anyContext->source.get_token()),
   ...);
  co_await ThisCoro::Suspend();
  co_return std::move(anyContext->value);
}

template <typename... TaskTypes>
[[nodiscard]] Task<AnyResult<TaskTypes...>> WhenAny(Task<TaskTypes>... tasks) {
  return WhenAnyImpl(std::make_index_sequence<sizeof...(tasks)>(),
                     std::move(tasks)...);
}

}  // namespace Cold

#endif /* COLD_COROUTINES_WHENANY */
