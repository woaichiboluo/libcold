#ifndef COLD_COROUTINES_CHANNEL
#define COLD_COROUTINES_CHANNEL

#include <deque>

#include "Condition.h"
#include "Task.h"
#include "ThisCoro.h"

namespace Cold {

template <typename T, typename Container>
concept ChannelContainer = requires(Container c) {
  c.push_back(std::declval<T>());
  c.front();
  c.pop_front();
};

template <typename T, typename Container = std::deque<T>>
requires ChannelContainer<T, Container>
class Channel {
 public:
  Channel() = default;
  ~Channel() = default;

  Channel(const Channel&) = delete;
  Channel& operator=(const Channel&) = delete;

  [[nodiscard]] Task<> Write(const T& value) {
    if (closed_) co_return;
    auto lock = co_await mutex_.ScopedLock();
    buffer_.push_back(value);
    condition_.NotifyOne();
  }

  [[nodiscard]] Task<> Write(T&& value) {
    if (closed_) co_return;
    auto lock = co_await mutex_.ScopedLock();
    buffer_.push_back(std::move(value));
    condition_.NotifyOne();
  }

  [[nodiscard]] Task<T> Read() {
    auto lock = co_await mutex_.ScopedLock();
    co_await condition_.Wait(lock,
                             [this] { return !buffer_.empty() || closed_; });
    auto& context = co_await ThisCoro::GetIoContext();
    co_await context.RunInThisContext();
    if (closed_) co_return T();
    T value = std::move(buffer_.front());
    buffer_.pop_front();
    co_return value;
  }

  [[nodiscard]] Task<> Close() {
    auto lock = co_await mutex_.ScopedLock();
    closed_ = true;
    condition_.NotifyAll();
  }

  bool IsClosed() const { return closed_; }

 private:
  AsyncMutex mutex_;
  std::atomic<bool> closed_ = false;
  Condition condition_;
  std::deque<T> buffer_;
};

}  // namespace Cold

#endif /* COLD_COROUTINES_CHANNEL */
