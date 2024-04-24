#include "cold/time/Time.h"

#include <cassert>

namespace Cold::Base {

bool Time::FromExploded(bool isLocal, const TimeExploded& ex, Time* time) {
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

void Time::Exploded(TimeExploded* ex, bool isLocal) const {
  const auto milliseconds = TimeSinceEpochMilliSeconds();
  int64_t seconds = milliseconds / kMillisecondsPerSecond;
  int64_t milli = milliseconds % kMillisecondsPerSecond;

  struct tm t;
  if (isLocal) {
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

std::string Time::Dump(bool isLocal) const {
  TimeExploded ex;
  char buf[64];
  if (isLocal) {
    LocalExplode(&ex);
  } else {
    UTCExplode(&ex);
  }
  auto ret =
      snprintf(buf, sizeof buf, "%04d-%02d-%02d %02d:%02d:%02d.%03d", ex.year,
               ex.month, ex.day, ex.hour, ex.minute, ex.second, ex.millisecond);
  assert(ret > 0);
  return {buf, static_cast<size_t>(ret)};
}

struct timespec Time::ToTimespec() const {
  struct timespec ts;
  using std::chrono::duration_cast;
  using std::chrono::nanoseconds;
  auto nano = duration_cast<nanoseconds>(timePoint_.time_since_epoch()).count();
  ts.tv_sec = nano / Time::kNanoSecondsPerSecond;
  ts.tv_nsec = nano - ts.tv_sec * Time::kNanoSecondsPerSecond;
  return ts;
}

}  // namespace Cold::Base