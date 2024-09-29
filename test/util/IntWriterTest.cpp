#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <deque>
#include <list>
#include <vector>

#include "cold/Cold.h"
#include "third_party/doctest.h"

using namespace Cold;

TEST_CASE("for char[]") {
  char buf[1024];
  int8_t i8 = 0x12;
  uint8_t u8 = 0x34;
  auto e = WriteInt(i8, buf);
  CHECK(e == buf + 1);
  e = WriteInt(u8, e);
  CHECK(e == buf + 2);
  int8_t i8r;
  e = ReadInt(i8r, buf);
  CHECK(e == buf + 1);
  CHECK(i8r == i8);
  uint8_t u8r;
  e = ReadInt(u8r, e);
  CHECK(e == buf + 2);
  CHECK(u8r == u8);

  int16_t i16 = 0x1234;
  uint16_t u16 = 0x5678;
  e = WriteInt(i16, buf);
  CHECK(e == buf + 2);
  e = WriteInt(u16, e);
  CHECK(e == buf + 4);
  int16_t i16r;
  e = ReadInt(i16r, buf);
  CHECK(e == buf + 2);
  CHECK(i16r == i16);
  uint16_t u16r;
  e = ReadInt(u16r, e);
  CHECK(e == buf + 4);
  CHECK(u16r == u16);

  int32_t i32 = -1;
  uint32_t u32 = 0x9abcdef0;
  e = WriteInt(i32, buf);
  CHECK(e == buf + 4);
  e = WriteInt(u32, e);
  CHECK(e == buf + 8);
  int32_t i32r;
  e = ReadInt(i32r, buf);
  CHECK(e == buf + 4);
  CHECK(i32r == i32);
  uint32_t u32r;
  e = ReadInt(u32r, e);
  CHECK(e == buf + 8);
  CHECK(u32r == u32);

  int64_t i64 = 0x123456789abcdef0;
  uint64_t u64 = 0x123456789abcdef0;
  e = WriteInt(i64, buf);
  CHECK(e == buf + 8);
  e = WriteInt(u64, e);
  CHECK(e == buf + 16);
  int64_t i64r;
  e = ReadInt(i64r, buf);
  CHECK(e == buf + 8);
  CHECK(i64r == i64);
  uint64_t u64r;
  e = ReadInt(u64r, e);
  CHECK(e == buf + 16);
  CHECK(u64r == u64);
}

TEST_CASE("for vector") {
  std::vector<unsigned char> vec;
  vec.resize(1024);
  int8_t i8 = 0x12;
  uint8_t u8 = 0x34;

  auto e = WriteInt(i8, vec.begin());
  CHECK(e == vec.begin() + 1);
  e = WriteInt(u8, e);
  CHECK(e == vec.begin() + 2);
  int8_t i8r;
  e = ReadInt(i8r, vec.begin());
  CHECK(e == vec.begin() + 1);
  CHECK(i8r == i8);
  uint8_t u8r;
  e = ReadInt(u8r, e);
  CHECK(e == vec.begin() + 2);
  CHECK(u8r == u8);

  int16_t i16 = 0x1234;
  uint16_t u16 = 0x5678;
  e = WriteInt(i16, vec.begin());
  CHECK(e == vec.begin() + 2);
  e = WriteInt(u16, e);
  CHECK(e == vec.begin() + 4);
  int16_t i16r;
  e = ReadInt(i16r, vec.begin());
  CHECK(e == vec.begin() + 2);
  CHECK(i16r == i16);
  uint16_t u16r;
  e = ReadInt(u16r, e);
  CHECK(e == vec.begin() + 4);
  CHECK(u16r == u16);

  int32_t i32 = 0x12345678;
  uint32_t u32 = 0x9abcdef0;
  e = WriteInt(i32, vec.begin());
  CHECK(e == vec.begin() + 4);
  e = WriteInt(u32, e);
  CHECK(e == vec.begin() + 8);
  int32_t i32r;
  e = ReadInt(i32r, vec.begin());
  CHECK(e == vec.begin() + 4);
  CHECK(i32r == i32);
  uint32_t u32r;
  e = ReadInt(u32r, e);
  CHECK(e == vec.begin() + 8);
  CHECK(u32r == u32);

  int64_t i64 = 0x123456789abcdef0;
  uint64_t u64 = 0x123456789abcdef0;
  e = WriteInt(i64, vec.begin());
  CHECK(e == vec.begin() + 8);
  e = WriteInt(u64, e);
  CHECK(e == vec.begin() + 16);
  int64_t i64r;
  e = ReadInt(i64r, vec.begin());
  CHECK(e == vec.begin() + 8);
  CHECK(i64r == i64);
  uint64_t u64r;
  e = ReadInt(u64r, e);
  CHECK(e == vec.begin() + 16);
  CHECK(u64r == u64);
}

TEST_CASE("for deque") {
  std::deque<int8_t> deq;
  deq.resize(1024);

  int8_t i8 = 0x12;
  uint8_t u8 = 0x34;

  auto e = WriteInt(i8, deq.begin());

  CHECK(e == deq.begin() + 1);
  e = WriteInt(u8, e);
  CHECK(e == deq.begin() + 2);
  int8_t i8r;
  e = ReadInt(i8r, deq.begin());
  CHECK(e == deq.begin() + 1);
  CHECK(i8r == i8);
  uint8_t u8r;
  e = ReadInt(u8r, e);
  CHECK(e == deq.begin() + 2);
  CHECK(u8r == u8);

  int16_t i16 = 0x1234;
  uint16_t u16 = 0x5678;
  e = WriteInt(i16, deq.begin());
  CHECK(e == deq.begin() + 2);
  e = WriteInt(u16, e);
  CHECK(e == deq.begin() + 4);
  int16_t i16r;
  e = ReadInt(i16r, deq.begin());

  CHECK(e == deq.begin() + 2);
  CHECK(i16r == i16);
  uint16_t u16r;
  e = ReadInt(u16r, e);
  CHECK(e == deq.begin() + 4);
  CHECK(u16r == u16);

  int32_t i32 = 0x12345678;
  uint32_t u32 = 0x9abcdef0;
  e = WriteInt(i32, deq.begin());
  CHECK(e == deq.begin() + 4);
  e = WriteInt(u32, e);
  CHECK(e == deq.begin() + 8);
  int32_t i32r;
  e = ReadInt(i32r, deq.begin());
  CHECK(e == deq.begin() + 4);
  CHECK(i32r == i32);
  uint32_t u32r;
  e = ReadInt(u32r, e);
  CHECK(e == deq.begin() + 8);
  CHECK(u32r == u32);

  int64_t i64 = 0x123456789abcdef0;
  uint64_t u64 = 0x123456789abcdef0;
  e = WriteInt(i64, deq.begin());
  CHECK(e == deq.begin() + 8);
  e = WriteInt(u64, e);
  CHECK(e == deq.begin() + 16);
  int64_t i64r;
  e = ReadInt(i64r, deq.begin());
  CHECK(e == deq.begin() + 8);
  CHECK(i64r == i64);
  uint64_t u64r;
  e = ReadInt(u64r, e);
  CHECK(e == deq.begin() + 16);
  CHECK(u64r == u64);
}

auto g_forward(auto it, int step) {
  std::advance(it, step);
  return it;
}

TEST_CASE("for list") {
  std::list<uint8_t> list;
  list.resize(1024);
  int8_t i8 = 0x12;
  uint8_t u8 = 0x34;

  auto e = WriteInt(i8, list.begin());
  CHECK(e == g_forward(list.begin(), 1));
  e = WriteInt(u8, e);
  CHECK(e == g_forward(list.begin(), 2));
  int8_t i8r;
  e = ReadInt(i8r, list.begin());
  CHECK(e == g_forward(list.begin(), 1));
  CHECK(i8r == i8);
  uint8_t u8r;
  e = ReadInt(u8r, e);
  CHECK(e == g_forward(list.begin(), 2));
  CHECK(u8r == u8);

  int16_t i16 = 0x1234;
  uint16_t u16 = 0x5678;
  e = WriteInt(i16, list.begin());
  CHECK(e == g_forward(list.begin(), 2));
  e = WriteInt(u16, e);
  CHECK(e == g_forward(list.begin(), 4));
  int16_t i16r;
  e = ReadInt(i16r, list.begin());
  CHECK(e == g_forward(list.begin(), 2));
  CHECK(i16r == i16);
  uint16_t u16r;
  e = ReadInt(u16r, e);
  CHECK(e == g_forward(list.begin(), 4));
  CHECK(u16r == u16);

  int32_t i32 = 0x12345678;
  uint32_t u32 = 0x9abcdef0;
  e = WriteInt(i32, list.begin());
  CHECK(e == g_forward(list.begin(), 4));
  e = WriteInt(u32, e);
  CHECK(e == g_forward(list.begin(), 8));
  int32_t i32r;
  e = ReadInt(i32r, list.begin());
  CHECK(e == g_forward(list.begin(), 4));
  CHECK(i32r == i32);
  uint32_t u32r;
  e = ReadInt(u32r, e);
  CHECK(e == g_forward(list.begin(), 8));
  CHECK(u32r == u32);

  int64_t i64 = 0x123456789abcdef0;
  uint64_t u64 = 0x123456789abcdef0;
  e = WriteInt(i64, list.begin());
  CHECK(e == g_forward(list.begin(), 8));
  e = WriteInt(u64, e);
  CHECK(e == g_forward(list.begin(), 16));
  int64_t i64r;
  e = ReadInt(i64r, list.begin());
  CHECK(e == g_forward(list.begin(), 8));
  CHECK(i64r == i64);
  uint64_t u64r;
  e = ReadInt(u64r, e);
  CHECK(e == g_forward(list.begin(), 16));
  CHECK(u64r == u64);
}