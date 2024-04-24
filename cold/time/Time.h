#ifndef COLD_TIME_TIME
#define COLD_TIME_TIME

#include <chrono>
#include <string>

namespace Cold::Base {

class Time {
 public:
  using Clock = std::chrono::system_clock;
  using TimePoint = Clock::time_point;

  constexpr static int64_t kMillisecondsPerSecond = 1000ll;
  constexpr static int64_t kMicrosecondsPerSecond =
      1000 * kMillisecondsPerSecond;
  constexpr static int64_t kNanoSecondsPerSecond =
      1000 * kMicrosecondsPerSecond;

  struct TimeExploded {
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

  static Time Now() { return Time(Clock::now()); }

  static Time FromTimeT(time_t t) { return Time(Clock::from_time_t(t)); }

  static bool FromUTCExplode(const TimeExploded& ex, Time* time) {
    return FromExploded(false, ex, time);
  }
  static bool FromLocalExplode(const TimeExploded& ex, Time* time) {
    return FromExploded(true, ex, time);
  }

  void UTCExplode(TimeExploded* ex) const { Exploded(ex, false); }
  void LocalExplode(TimeExploded* ex) const { Exploded(ex, true); }

  // time since epoch
  auto TimeSinceEpochDuration() const { return timePoint_.time_since_epoch(); }
  constexpr int64_t TimeSinceEpochSeconds() const {
    using namespace std::chrono;
    return duration_cast<seconds>(timePoint_.time_since_epoch()).count();
  }
  constexpr int64_t TimeSinceEpochMilliSeconds() const {
    using namespace std::chrono;
    return duration_cast<milliseconds>(timePoint_.time_since_epoch()).count();
  }
  constexpr time_t ToTimeT() const { return TimeSinceEpochSeconds(); }

  struct timespec ToTimespec() const;

  template <typename Rep, class Period>
  constexpr Time operator+(std::chrono::duration<Rep, Period> duration) const {
    return Time(timePoint_ + duration);
  }

  template <typename Rep, class Period>
  constexpr Time operator-(std::chrono::duration<Rep, Period> duration) const {
    return Time(timePoint_ - duration);
  }

  template <typename Rep, class Period>
  constexpr Time& operator+=(std::chrono::duration<Rep, Period> duration) {
    timePoint_ += duration;
    return *this;
  }

  template <typename Rep, class Period>
  constexpr Time& operator-=(std::chrono::duration<Rep, Period> duration) {
    timePoint_ -= duration;
    return *this;
  }

  constexpr TimePoint::duration operator-(const Time& other) const {
    return timePoint_ - other.timePoint_;
  }

  constexpr auto operator<=>(const Time&) const = default;

  constexpr TimePoint GetStdTimePoint() const { return timePoint_; }

  std::string Dump(bool isLocal = true) const;

 private:
  static bool FromExploded(bool isLocal, const TimeExploded& exploded,
                           Time* time);

  void Exploded(TimeExploded* ex, bool local) const;

  TimePoint timePoint_;
};  // class Time

template <typename Duration>
Time operator+(Duration lhs, const Time& rhs) {
  return rhs + lhs;
}

}  // namespace Cold::Base

#endif /* COLD_TIME_TIME */
