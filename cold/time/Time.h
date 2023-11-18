#ifndef COLD_TIME_TIME
#define COLD_TIME_TIME

#include <chrono>
#include <string_view>

namespace Cold::Base {

class Time {
 public:
  using Clock = std::chrono::system_clock;
  using TimePoint = Clock::time_point;

  struct Exploded {
    int year;         // 20xx
    int month;        // (1-12)
    int day;          // 1-based day of month (1-31)
    int dayOfWeek;    // 1-based day of week (1 - 7) Sunday:7
    int dayInYear;    // (1 - 366)
    int hour;         // (0-23)
    int minute;       // (0-59)
    int second;       // (0 - 59)
    int millisecond;  // (0-999)
  };

  constexpr Time() = default;
  constexpr explicit Time(TimePoint p) : timePoint_(p) {}
  ~Time() = default;

  static Time now() { return Time(Clock::now()); }

  static Time fromTimeT(time_t t) { return Time(Clock::from_time_t(t)); }

  static bool fromUTCExplode(const Exploded& ex, Time* time) {
    return fromExploded(false, ex, time);
  }
  static bool fromLocalExplode(const Exploded& ex, Time* time) {
    return fromExploded(true, ex, time);
  }

  void UTCExplode(Exploded* ex) const { exploded(ex, false); }
  void localExplode(Exploded* ex) const { exploded(ex, true); }

  // time since epoch
  auto timeSinceEpochDuration() const { return timePoint_.time_since_epoch(); }
  constexpr int64_t timeSinceEpochSeconds() const {
    using namespace std::chrono;
    return duration_cast<seconds>(timePoint_.time_since_epoch()).count();
  }
  constexpr int64_t timeSinceEpochMilliSeconds() const {
    using namespace std::chrono;
    return duration_cast<milliseconds>(timePoint_.time_since_epoch()).count();
  }
  constexpr time_t toTimeT() const { return timeSinceEpochSeconds(); }

  template <typename Duration>
  constexpr Time operator+(Duration duration) const {
    return Time(timePoint_ + duration);
  }

  template <typename Duration>
  constexpr Time operator-(Duration duration) const {
    return Time(timePoint_ - duration);
  }

  template <typename Duration>
  constexpr Time& operator+=(Duration duration) {
    timePoint_ += duration;
    return *this;
  }

  template <typename Duration>
  constexpr Time& operator-=(Duration duration) {
    timePoint_ -= duration;
    return *this;
  }

  TimePoint::duration operator-(const Time& other) {
    return timePoint_ - other.timePoint_;
  }

  constexpr auto operator<=>(const Time&) const = default;

  constexpr TimePoint getStdTimePoint() const { return timePoint_; }

  // without cache
  // debug use,log don't use this
  std::string dump(bool isLocal = true) const;

 private:
  constexpr static int64_t kMillisecondsPerSecond = 1000;

  static bool fromExploded(bool isLocal, const Exploded& exploded, Time* time);

  void exploded(Exploded* ex, bool local) const;

  TimePoint timePoint_;
};  // class Time

template <typename Duration>
Time operator+(Duration lhs, const Time& rhs) {
  return rhs + lhs;
}

}  // namespace Cold::Base

#endif /* COLD_TIME_TIME */
