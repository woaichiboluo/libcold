#ifndef COLD_UTIL_SHELLCOLOR
#define COLD_UTIL_SHELLCOLOR

#include <cstdint>
#include <string>
#include <string_view>

#include "cold/util/StringUtil.h"

#define EMPHASIS_MAP(XX) \
  XX(1, Bold)            \
  XX(4, Underscore)      \
  XX(5, Blink)           \
  XX(7, Reverse)         \
  XX(8, Concealed)

#define TERMINAL_TEXT_COLOR_MAP(XX) \
  XX(30, black)                     \
  XX(90, darkGrey)                  \
  XX(31, red)                       \
  XX(91, lightRed)                  \
  XX(32, green)                     \
  XX(92, lightGreen)                \
  XX(33, Yellow)                    \
  XX(93, yellow)                    \
  XX(34, blue)                      \
  XX(94, lightBlue)                 \
  XX(35, magenta)                   \
  XX(95, lightPurple)               \
  XX(36, cyan)                      \
  XX(96, turquoise)                 \
  XX(37, White)                     \
  XX(97, white)

#define TERMINAL_TEXT_BG_COLOR_MAP(XX) \
  XX(40, black)                        \
  XX(100, darkGrey)                    \
  XX(41, red)                          \
  XX(101, lightRed)                    \
  XX(42, green)                        \
  XX(102, lightGreen)                  \
  XX(43, Yellow)                       \
  XX(103, yellow)                      \
  XX(44, blue)                         \
  XX(104, lightBlue)                   \
  XX(45, magenta)                      \
  XX(105, lightPurple)                 \
  XX(46, cyan)                         \
  XX(106, turquoise)                   \
  XX(47, White)                        \
  XX(107, white)

namespace Cold::Base::Color {

enum class Emphasis : uint8_t {
#define XX(value, name) name = value,
  EMPHASIS_MAP(XX)
#undef XX
};

enum class TerminalTextColor : uint8_t {
#define XX(value, name) name = value,
  TERMINAL_TEXT_COLOR_MAP(XX)
#undef XX
};

enum class TerminalBackgroundColor : uint8_t {
#define XX(value, name) name = value,
  TERMINAL_TEXT_BG_COLOR_MAP(XX)
#undef XX
};

constexpr std::string_view ResetColor = "\033[0m";

class TerminalColorPrefix {
 public:
  constexpr TerminalColorPrefix(TerminalTextColor textColor) {
    auto end_ = buf_ + 2;
    end_ = PadTextColor(end_, textColor);
    *end_++ = 'm';
    *end_ = 0;
  }

  constexpr TerminalColorPrefix(TerminalTextColor textColor,
                                TerminalBackgroundColor bgColor) {
    auto end_ = buf_ + 2;
    end_ = PadBgColor(end_, bgColor);
    *end_++ = ';';
    end_ = PadTextColor(end_, textColor);
    *end_++ = 'm';
    *end_ = 0;
  }

  constexpr TerminalColorPrefix(TerminalTextColor textColor,
                                TerminalBackgroundColor bgColor,
                                Emphasis emphasis) {
    auto end_ = buf_ + 2;
    end_ = PadBgColor(end_, bgColor);
    *end_++ = ';';
    end_ = PadBgColor(end_, bgColor);
    *end_++ = ';';
    end_ = PadTextColor(end_, textColor);
    *end_++ = 'm';
    *end_ = 0;
  }

  constexpr char* PadTextColor(char* begin, TerminalTextColor textColor) {
    uint8_t v = static_cast<uint8_t>(textColor);
    begin[0] = static_cast<char>(v / 10 + '0');
    begin[1] = static_cast<char>(v % 10 + '0');
    return begin + 2;
  }

  constexpr char* PadBgColor(char* begin, TerminalBackgroundColor bgColor) {
    uint8_t v = static_cast<uint8_t>(bgColor);
    if (v >= 100) {
      begin[0] = static_cast<char>(v / 100 + '0');
      begin[1] = static_cast<char>(v / 10 % 10 + '0');
      begin[2] = static_cast<char>(v % 10 + '0');
      return begin + 3;
    } else {
      begin[0] = static_cast<char>(v / 10 + '0');
      begin[1] = static_cast<char>(v % 10 + '0');
      return begin + 2;
    }
  }

  constexpr char* PadEmphasis(char* begin, Emphasis emphasis) {
    uint8_t v = static_cast<uint8_t>(emphasis);
    begin[0] = static_cast<char>(v + '0');
    return begin + 1;
  }

  constexpr const char* data() const { return buf_; }

 private:
  char buf_[64] = "\033[";
};

inline std::string GetColorStr(TerminalColorPrefix& color,
                               std::string_view content) {
  std::string ret(color.data());
  return ret.append(content).append(ResetColor);
}

};  // namespace Cold::Base::Color

#endif /* COLD_UTIL_SHELLCOLOR */
