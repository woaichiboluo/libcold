#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <cstdio>
#include <ctime>

#include "cold/time/Time.h"
#include "third_party/doctest.h"

using namespace Cold;

TEST_CASE("default constructor") {
  using namespace std::chrono;
  Time t;
  CHECK(duration_cast<seconds>(t.TimeSinceEpochDuration()).count() == 0);
  CHECK(t.GetStdTimePoint() == system_clock::time_point());
  CHECK(t.TimeSinceEpochSeconds() == 0);
  CHECK(t.Dump(false) == "1970-01-01 00:00:00.000");
}

TEST_CASE("time_t convert") {
  CHECK(Time::FromTimeT(100).ToTimeT() == 100);
  CHECK(Time::FromTimeT(0).ToTimeT() == 0);
  CHECK(Time().ToTimeT() == 0);
}

TEST_CASE("test operator") {
  using namespace std::chrono;
  auto t = Time::Now();
  auto d = t.TimeSinceEpochDuration();
  CHECK(d + seconds(5) == (t + seconds(5)).TimeSinceEpochDuration());
  CHECK(d + nanoseconds(100) ==
        (t + nanoseconds(100)).TimeSinceEpochDuration());
  CHECK(d + milliseconds(100) ==
        (t + milliseconds(100)).TimeSinceEpochDuration());
  CHECK(d + microseconds(100) ==
        (t + microseconds(100)).TimeSinceEpochDuration());
  auto n = seconds(20) + t;
  CHECK(t < n);
  CHECK(t <= n);
  CHECK(n - t == seconds(20));
  t += seconds(20);
  CHECK(t == n);
  t -= seconds(100);
  CHECK(n > t);
  CHECK(n >= t);
  CHECK(t + seconds(100) == n);
  CHECK(n - t == seconds(100));
}

TEST_CASE("utc explode") {
  auto t = time(nullptr);
  struct tm tt;
  gmtime_r(&t, &tt);
  Time::TimeExploded ex;
  Time::FromTimeT(t).UTCExplode(&ex);
  CHECK(ex.year == tt.tm_year + 1900);
  CHECK(ex.month == tt.tm_mon + 1);
  CHECK(ex.day == tt.tm_mday);
  CHECK(ex.hour == tt.tm_hour);
  CHECK(ex.minute == tt.tm_min);
  CHECK(ex.second == tt.tm_sec);
  Time l;
  CHECK(Time::FromUTCExplode(ex, &l) == true);
  CHECK(l.ToTimeT() == t);
}

TEST_CASE("local explode") {
  auto t = time(nullptr);
  struct tm tt;
  localtime_r(&t, &tt);
  Time::TimeExploded ex;
  Time::FromTimeT(t).LocalExplode(&ex);
  CHECK(ex.year == tt.tm_year + 1900);
  CHECK(ex.month == tt.tm_mon + 1);
  CHECK(ex.day == tt.tm_mday);
  CHECK(ex.hour == tt.tm_hour);
  CHECK(ex.minute == tt.tm_min);
  CHECK(ex.second == tt.tm_sec);
  Time l;
  CHECK(Time::FromLocalExplode(ex, &l) == true);
  CHECK(l.ToTimeT() == t);
}

TEST_CASE("from utc explode") {
  Time::TimeExploded ex{};
  ex.year = 2023;
  ex.month = 11;
  ex.day = 12;
  ex.hour = 11;
  ex.minute = 11;
  ex.second = 11;
  Time time;
  CHECK(Time::FromUTCExplode(ex, &time) == true);
  CHECK(time.Dump(false) == "2023-11-12 11:11:11.000");
  Time::TimeExploded ex1{};
  time.UTCExplode(&ex1);
  CHECK(ex.year == ex1.year);
  CHECK(ex.month == ex1.month);
  CHECK(ex.day == ex1.day);
  CHECK(ex.hour == ex1.hour);
  CHECK(ex.minute == ex1.minute);
  CHECK(ex.second == ex1.second);

  CHECK(ex1.dayInYear == 316);
  CHECK(ex1.dayOfWeek == 7);
}

TEST_CASE("from local explode") {
  Time::TimeExploded ex{};
  ex.year = 2023;
  ex.month = 11;
  ex.day = 11;
  ex.hour = 11;
  ex.minute = 11;
  ex.second = 11;
  Time time;
  CHECK(Time::FromLocalExplode(ex, &time) == true);
  CHECK(time.Dump() == "2023-11-11 11:11:11.000");

  Time::TimeExploded ex1{};
  time.LocalExplode(&ex1);
  CHECK(ex.year == ex1.year);
  CHECK(ex.month == ex1.month);
  CHECK(ex.day == ex1.day);
  CHECK(ex.hour == ex1.hour);
  CHECK(ex.minute == ex1.minute);
  CHECK(ex.second == ex1.second);

  CHECK(ex1.dayInYear == 315);
  CHECK(ex1.dayOfWeek == 6);
}
