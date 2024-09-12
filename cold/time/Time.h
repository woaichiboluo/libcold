#ifndef COLD_TIME_TIME
#define COLD_TIME_TIME

#include <cassert>
#include <chrono>
#include <string>

namespace Cold {

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

  struct timespec ToTimespec() const {
    struct timespec ts;
    using std::chrono::duration_cast;
    using std::chrono::nanoseconds;
    auto nano =
        duration_cast<nanoseconds>(timePoint_.time_since_epoch()).count();
    ts.tv_sec = nano / Time::kNanoSecondsPerSecond;
    ts.tv_nsec = nano - ts.tv_sec * Time::kNanoSecondsPerSecond;
    return ts;
  }

  template <typename REP, typename PERIOD>
  constexpr Time operator+(std::chrono::duration<REP, PERIOD> duration) const {
    return Time(timePoint_ + duration);
  }

  template <typename REP, typename PERIOD>
  constexpr Time operator-(std::chrono::duration<REP, PERIOD> duration) const {
    return Time(timePoint_ - duration);
  }

  template <typename REP, typename PERIOD>
  constexpr Time& operator+=(std::chrono::duration<REP, PERIOD> duration) {
    timePoint_ += duration;
    return *this;
  }

  template <typename REP, typename PERIOD>
  constexpr Time& operator-=(std::chrono::duration<REP, PERIOD> duration) {
    timePoint_ -= duration;
    return *this;
  }

  constexpr TimePoint::duration operator-(const Time& other) const {
    return timePoint_ - other.timePoint_;
  }

  constexpr auto operator<=>(const Time&) const = default;

  constexpr TimePoint GetStdTimePoint() const { return timePoint_; }

  std::string Dump(bool isLocal = true) const {
    TimeExploded ex;
    char buf[64];
    if (isLocal) {
      LocalExplode(&ex);
    } else {
      UTCExplode(&ex);
    }
    auto ret = snprintf(buf, sizeof buf, "%04d-%02d-%02d %02d:%02d:%02d.%03d",
                        ex.year, ex.month, ex.day, ex.hour, ex.minute,
                        ex.second, ex.millisecond);
    assert(ret > 0);
    return {buf, static_cast<size_t>(ret)};
  }

 private:
  static bool FromExploded(bool isLocal, const TimeExploded& ex, Time* time) {
    struct tm t {};
    t.tm_year = ex.year - 1900;
    t.tm_mon = ex.month - 1;
    t.tm_mday = ex.day;
    t.tm_hour = ex.hour;
    t.tm_min = ex.minute;
    t.tm_sec = ex.second;
    t.tm_wday = 0;  // ignore
    t.tm_yday = 0;  // ignore
    t.tm_isdst = -1;

    time_t seconds = 0;
    seconds = isLocal ? mktime(&t) : timegm(&t);
    if (seconds != -1) {
      *time = Time::FromTimeT(seconds);
      return true;
    }
    return false;
  }

  void Exploded(TimeExploded* ex, bool local) const {
    const auto milliseconds = TimeSinceEpochMilliSeconds();
    int64_t seconds = milliseconds / kMillisecondsPerSecond;
    int64_t milli = milliseconds % kMillisecondsPerSecond;

    struct tm t;
    if (local) {
      localtime_r(&seconds, &t);
    } else {
      gmtime_r(&seconds, &t);
    }

    ex->year = t.tm_year + 1900;
    ex->month = t.tm_mon + 1;
    ex->day = t.tm_mday;
    ex->dayOfWeek = t.tm_wday == 0 ? 7 : t.tm_wday;
    ex->dayInYear = t.tm_yday + 1;
    ex->hour = t.tm_hour;
    ex->minute = t.tm_min;
    ex->second = t.tm_sec;
    ex->millisecond = static_cast<int>(milli);
  }

  TimePoint timePoint_;
};  // class Time

template <typename Duration>
Time operator+(Duration lhs, const Time& rhs) {
  return rhs + lhs;
}

}  // namespace Cold

#endif
