#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <limits>

#include "cold/util/StringUtil.h"
#include "third_party/doctest.h"
using namespace Cold;

TEST_CASE("test split") {
  std::string seq1 = "hello,world";
  auto converter = [](const auto& list) {
    std::vector<std::string> res;
    for (const auto& l : list) res.emplace_back(l);
    return res;
  };
  auto l1 = Base::splitToString(seq1, ",");
  CHECK(converter(Base::splitToStringView(seq1, ",")) == l1);
  CHECK(l1.size() == 2);
  CHECK(l1[0] == "hello");
  CHECK(l1[1] == "world");

  std::string seq2 = "1 2 3 4 5 6 7 8 9 10   ";
  l1 = Base::splitToString(seq2, " ");
  CHECK(converter(Base::splitToStringView(seq2, " ")) == l1);
  CHECK(l1.size() == 13);
  for (size_t i = 0; i < 9; ++i) {
    CHECK(l1[i].size() == 1);
    CHECK(l1[i][0] == ('1' + i));
  }
  CHECK(l1[9] == "10");
  CHECK(l1[10].size() == 0);
  CHECK(l1[10] == l1[11]);
  CHECK(l1[11] == l1[12]);
  CHECK(l1[10] == "");

  std::string seq3 =
      "hello1hellotwohello Three hello&four&hello||five||hellohello";
  l1 = Base::splitToString(seq3, "xxx");
  CHECK(converter(Base::splitToString(seq3, "xxx")) == l1);
  CHECK(l1.size() == 1);
  CHECK(l1[0] == seq3);
  l1 = Base::splitToString(seq3, "hello");
  CHECK(converter(Base::splitToStringView(seq3, "hello")) == l1);
  CHECK(l1.size() == 8);
  CHECK(l1[0] == "");
  CHECK(l1[1] == "1");
  CHECK(l1[2] == "two");
  CHECK(l1[3] == " Three ");
  CHECK(l1[4] == "&four&");
  CHECK(l1[5] == "||five||");
  CHECK(l1[6] == "");
  CHECK(l1[7] == "");

  std::string seq4 = "hello";
  l1 = Base::splitToString(seq4, "hello");
  CHECK(converter(Base::splitToStringView(seq4, "hello")) == l1);
  CHECK(l1.size() == 2);
  CHECK(l1[0] == "");
  CHECK(l1[1] == "");
}

TEST_CASE("test number string conversion") {
  CHECK(Base::intToStr(0) == "0");
  CHECK(Base::intToStr(-1) == "-1");
  CHECK(Base::intToStr(-3, 2) == "-11");
  CHECK(Base::intToStr(+1) == "1");

  CHECK(Base::intToStr(std::numeric_limits<int8_t>::max()) == "127");
  CHECK(Base::intToStr(std::numeric_limits<int8_t>::min()) == "-128");
  CHECK(Base::intToStr(std::numeric_limits<uint8_t>::max()) == "255");

  CHECK(Base::intToStr(std::numeric_limits<int16_t>::max()) == "32767");
  CHECK(Base::intToStr(std::numeric_limits<int16_t>::min()) == "-32768");
  CHECK(Base::intToStr(std::numeric_limits<uint16_t>::max()) == "65535");

  CHECK(Base::intToStr(std::numeric_limits<int32_t>::max()) == "2147483647");
  CHECK(Base::intToStr(std::numeric_limits<int32_t>::min()) == "-2147483648");
  CHECK(Base::intToStr(std::numeric_limits<uint32_t>::max()) == "4294967295");

  CHECK(Base::intToStr(std::numeric_limits<int64_t>::max()) ==
        "9223372036854775807");
  CHECK(Base::intToStr(std::numeric_limits<int64_t>::min()) ==
        "-9223372036854775808");
  CHECK(Base::intToStr(std::numeric_limits<uint64_t>::max()) ==
        "18446744073709551615");

  //当format为fixed时，precision代表保留几位小数,会四舍五入
  CHECK(Base::floatToStr(0.00f, 0) == "0");
  CHECK(Base::floatToStr(0.00f, 2) == "0.00");
  CHECK(Base::floatToStr(0.00, 0) == "0");
  CHECK(Base::floatToStr(0.00, 2) == "0.00");

  CHECK(Base::floatToStr(3.1415926, 2) == "3.14");
  CHECK(Base::floatToStr(3.1415926, 3) == "3.142");
  CHECK(Base::floatToStr(3.1415926, 7) == "3.1415926");

  CHECK(Base::floatToStr(2.718281828459045, 2) == "2.72");
  CHECK(Base::floatToStr(2.718281828459045, 3) == "2.718");
  CHECK(Base::floatToStr(2.718281828459045, 4) == "2.7183");
  CHECK(Base::floatToStr(2.718281828459045, 15) == "2.718281828459045");

  CHECK(Base::numberToInternalByteStr(8) == "1000");
  CHECK(Base::numberToInternalByteStr(8, true) == "0b1000");
  CHECK(Base::numberToInternalByteStr(8, true, true) ==
        "0b00000000000000000000000000001000");
  CHECK(Base::numberToInternalByteStr(-1) ==
        "11111111111111111111111111111111");
  CHECK(Base::numberToInternalByteStr(-1, true) ==
        "0b11111111111111111111111111111111");
  CHECK(Base::numberToInternalHexStr(-1) == "ffffffff");
  CHECK(Base::numberToInternalHexStr(-1, true) == "0xffffffff");

  CHECK(Base::numberToInternalByteStr(0) == "0");
  CHECK(Base::numberToInternalByteStr(0, true) == "0b0");
  CHECK(Base::numberToInternalByteStr(0, false, true) ==
        "00000000000000000000000000000000");
  CHECK(Base::numberToInternalByteStr(0, true, true) ==
        "0b00000000000000000000000000000000");

  CHECK(Base::numberToInternalHexStr(0) == "0");
  CHECK(Base::numberToInternalHexStr(0, true) == "0x0");
  CHECK(Base::numberToInternalHexStr(0, false, true) == "00000000");
  CHECK(Base::numberToInternalHexStr(0, true, true) == "0x00000000");

  CHECK(Base::numberToInternalByteStr(std::numeric_limits<int8_t>::max()) ==
        "1111111");
  CHECK(Base::numberToInternalByteStr(std::numeric_limits<int8_t>::max(),
                                      true) == "0b1111111");
  CHECK(Base::numberToInternalByteStr(std::numeric_limits<int8_t>::max(), false,
                                      true) == "01111111");
  CHECK(Base::numberToInternalByteStr(std::numeric_limits<int8_t>::max(), true,
                                      true) == "0b01111111");

  CHECK(Base::numberToInternalByteStr(std::numeric_limits<int8_t>::min()) ==
        "10000000");
  CHECK(Base::numberToInternalByteStr(std::numeric_limits<int8_t>::min(),
                                      true) == "0b10000000");
  CHECK(Base::numberToInternalByteStr(std::numeric_limits<int8_t>::min(), false,
                                      true) == "10000000");
  CHECK(Base::numberToInternalByteStr(std::numeric_limits<int8_t>::min(), true,
                                      true) == "0b10000000");

  CHECK(Base::numberToInternalHexStr(std::numeric_limits<int8_t>::max()) ==
        "7f");
  CHECK(Base::numberToInternalHexStr(std::numeric_limits<int8_t>::max(),
                                     true) == "0x7f");
  CHECK(Base::numberToInternalHexStr(std::numeric_limits<int8_t>::max(), false,
                                     true) == "7f");
  CHECK(Base::numberToInternalHexStr(std::numeric_limits<int8_t>::max(), true,
                                     true) == "0x7f");

  CHECK(Base::numberToInternalHexStr(std::numeric_limits<int8_t>::min()) ==
        "80");
  CHECK(Base::numberToInternalHexStr(std::numeric_limits<int8_t>::min(),
                                     true) == "0x80");
  CHECK(Base::numberToInternalHexStr(std::numeric_limits<int8_t>::min(), false,
                                     true) == "80");
  CHECK(Base::numberToInternalHexStr(std::numeric_limits<int8_t>::min(), true,
                                     true) == "0x80");

  CHECK(Base::numberToInternalByteStr(std::numeric_limits<int64_t>::max()) ==
        "111111111111111111111111111111111111111111111111111111111111111");
  CHECK(Base::numberToInternalByteStr(std::numeric_limits<int64_t>::max(),
                                      true) ==
        "0b111111111111111111111111111111111111111111111111111111111111111");
  CHECK(Base::numberToInternalByteStr(std::numeric_limits<int64_t>::max(),
                                      false, true) ==
        "0111111111111111111111111111111111111111111111111111111111111111");
  CHECK(Base::numberToInternalByteStr(std::numeric_limits<int64_t>::max(), true,
                                      true) ==
        "0b0111111111111111111111111111111111111111111111111111111111111111");

  CHECK(Base::numberToInternalHexStr(std::numeric_limits<int64_t>::max()) ==
        "7fffffffffffffff");
  CHECK(Base::numberToInternalHexStr(std::numeric_limits<int64_t>::max(),
                                     true) == "0x7fffffffffffffff");
  CHECK(Base::numberToInternalHexStr(std::numeric_limits<int64_t>::max(), false,
                                     true) == "7fffffffffffffff");
  CHECK(Base::numberToInternalHexStr(std::numeric_limits<int64_t>::max(), true,
                                     true) == "0x7fffffffffffffff");

  CHECK(Base::numberToInternalByteStr(16ll) == "10000");
  CHECK(Base::numberToInternalByteStr(16ll, true) == "0b10000");
  CHECK(Base::numberToInternalByteStr(16ll, false, true) ==
        "0000000000000000000000000000000000000000000000000000000000010000");
  CHECK(Base::numberToInternalByteStr(16ll, true, true) ==
        "0b0000000000000000000000000000000000000000000000000000000000010000");

  CHECK(Base::numberToInternalHexStr(16ll) == "10");
  CHECK(Base::numberToInternalHexStr(16ll, true) == "0x10");
  CHECK(Base::numberToInternalHexStr(16ll, false, true) == "0000000000000010");
  CHECK(Base::numberToInternalHexStr(16ll, true, true) == "0x0000000000000010");
}

TEST_CASE("test trim") {
  std::string seq1 = "   abcdefgh          ";
  CHECK(Base::trimToString(seq1) == "abcdefgh");
  CHECK(Base::trimToString(seq1, 'x') == "   abcdefgh          ");
  std::string seq2 = "   abcdefgh";
  CHECK(Base::trimToString(seq2) == "abcdefgh");
  std::string seq3 = "abcdefgh";
  CHECK(Base::trimToString(seq3) == "abcdefgh");
  std::string seq4 = "xxxxwwww";
  CHECK(Base::trimToString(seq4) == "xxxxwwww");
  CHECK(Base::trimToString(seq4, 'x') == "wwww");
  CHECK(Base::trimToString(seq4, 'w') == "xxxx");
}