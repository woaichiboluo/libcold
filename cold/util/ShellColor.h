#ifndef COLD_UTIL_SHELLCOLOR
#define COLD_UTIL_SHELLCOLOR

#include <cstdint>

// man console_codes

// 31 - 37 91 - 97
#define SHELL_FRONT_COLOR_MAP(XX) \
  XX(31, kRed)                    \
  XX(32, kGreen)                  \
  XX(33, kYellow)                 \
  XX(34, kBlue)                   \
  XX(35, kMagenta)                \
  XX(36, kCyan)                   \
  XX(37, kWhite)                  \
  XX(90, kBrightBlack)            \
  XX(91, kBrightRed)              \
  XX(92, kBrightGreen)            \
  XX(93, kBrightYellow)           \
  XX(94, kBrightBlue)             \
  XX(95, kBrightMagenta)          \
  XX(96, kBrightCyan)             \
  XX(97, kBrightWhite)

// 39 - 47
#define SHELL_BACKGROUND_COLOR_MAP(XX) \
  XX(39, kDefault)                     \
  XX(40, kBlack)                       \
  XX(41, kRed)                         \
  XX(42, kGreen)                       \
  XX(43, kYellow)                      \
  XX(44, kBlue)                        \
  XX(45, kMagenta)                     \
  XX(46, kCyan)                        \
  XX(47, kWhite)

#define SHELL_EMPHASIS_MAP(XX) \
  XX(0, kReset)                \
  XX(1, kBold)                 \
  XX(2, kHalfBright)           \
  XX(4, kUnderScore)           \
  XX(5, kBlink)                \
  XX(7, kReverse)

namespace Cold::Base::Color {

enum class Emphasis : uint8_t {
#define XX(value, name) name = value,
  SHELL_EMPHASIS_MAP(XX)
#undef XX
};

enum class FrontColor : uint8_t {
#define XX(value, name) name = value,
  SHELL_FRONT_COLOR_MAP(XX)
#undef XX
};

enum class BackGroundColor : uint8_t {
#define XX(value, name) name = value,
  SHELL_BACKGROUND_COLOR_MAP(XX)
#undef XX
};

class ColorControl {
 public:
  constexpr ColorControl(FrontColor front) {
    auto end_ = buf_ + 2;
    end_ = Pad(end_, static_cast<uint8_t>(front));
    *end_++ = 'm';
    *end_ = 0;
  }

  constexpr ColorControl(BackGroundColor back) {
    auto end_ = buf_ + 2;
    end_ = Pad(end_, static_cast<uint8_t>(back));
    *end_++ = 'm';
    *end_ = 0;
  }

  constexpr ColorControl(FrontColor front, BackGroundColor back) {
    auto end_ = buf_ + 2;
    end_ = Pad(end_, static_cast<uint8_t>(front));
    *end_++ = ';';
    end_ = Pad(end_, static_cast<uint8_t>(back));
    *end_++ = 'm';
    *end_ = 0;
  }

  constexpr ColorControl(FrontColor front, BackGroundColor back, Emphasis e) {
    auto end_ = buf_ + 2;
    end_ = Pad(end_, static_cast<uint8_t>(front));
    *end_++ = ';';
    end_ = Pad(end_, static_cast<uint8_t>(back));
    *end_++ = ';';
    end_ = Pad(end_, static_cast<uint8_t>(e));
    *end_++ = 'm';
    *end_ = 0;
  }

  constexpr char* Pad(char* begin, uint8_t v) {
    *begin = static_cast<char>(v / 100 + '0');
    begin += (v / 100) > 0;
    *begin = static_cast<char>(v / 10 % 10 + '0');
    begin += (v / 10 % 10) > 0;
    *begin = static_cast<char>(v % 10 + '0');
    ++begin;
    return begin;
  }

  constexpr const char* ColorStart() const { return buf_; }

  static constexpr const char* ColorEnd() { return "\033[0m"; }

 private:
  char buf_[64] = "\033[";
};

}  // namespace Cold::Base::Color

#endif /* COLD_UTIL_SHELLCOLOR */
