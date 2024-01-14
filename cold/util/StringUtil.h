#ifndef COLD_UTIL_STRINGUTIL
#define COLD_UTIL_STRINGUTIL

#include <cassert>
#include <cctype>
#include <charconv>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <vector>

namespace Cold::Base {

namespace internal {

template <typename T>
std::vector<T> Split(std::string_view content, std::string_view separator) {
  std::vector<T> res;
  if (content.empty()) return res;

  size_t start = 0;
  while (start != std::string_view::npos) {
    size_t end = content.find(separator, start);
    if (end == std::string_view::npos) {
      res.emplace_back(content.substr(start));
      start = std::string_view::npos;
    } else {
      res.emplace_back(content.substr(start, end - start));
      start = end + separator.size();
    }
  }
  return res;
}

}  // namespace internal

[[nodiscard]] std::vector<std::string_view> SplitToStringView(
    std::string_view content, std::string_view separator);

[[nodiscard]] std::vector<std::string> SplitToString(
    std::string_view content, std::string_view separator);

//将字符串转换为整形
template <typename NumberType>
[[nodiscard]] bool StrToInt(
    std::string_view str, NumberType* pointer,
    int base = 10) requires std::is_integral_v<NumberType> {
  assert(2 <= base && base <= 35);
  NumberType value = 0;
  auto [ptr, ec] = std::from_chars(str.begin(), str.end(), value, base);
  if (ec != std::errc()) return false;
  if (pointer) *pointer = value;
  return true;
}

//将字符串转换为浮点数
template <typename NumberType>
[[nodiscard]] bool StrToFloat(
    std::string_view str, NumberType* pointer, int precision = 2,
    std::chars_format format = std::chars_format::fixed) requires
    std::is_floating_point_v<NumberType> {
  NumberType value = 0;
  auto [ptr, ec] =
      std::from_chars(str.begin(), str.end(), value, format, precision);
  if (ec != std::errc()) return false;
  if (pointer) *pointer = value;
  return true;
}

//把整数转换为字符串(非补码)
template <typename NumberType>
[[nodiscard]] std::string IntToStr(
    NumberType number, int base = 10) requires std::is_integral_v<NumberType> {
  char buf[128];
  assert(2 <= base && base <= 35);
  auto [ptr, ec] = std::to_chars(buf, buf + sizeof buf, number, base);
  assert(ec == std::errc() && ptr < buf + sizeof buf);
  return {buf, ptr};
}

//获取整数的byte串
template <typename NumberType>
[[nodiscard]] std::string NumberToInternalByteStr(
    NumberType number, bool needPrefix = false,
    bool pad = false) requires std::is_integral_v<NumberType> {
  constexpr size_t bitLens = sizeof(NumberType) * 8;
  char buf[bitLens + 2] = {'0', 'b'};
  char* start = buf + bitLens + 1;
  for (size_t i = 0; i < bitLens; ++i) {
    auto value = ((number >> i) & 1);
    assert(value == 0 || value == 1);
    buf[bitLens + 1 - i] = static_cast<char>(value + '0');
    if (!pad && value) {
      start = buf + bitLens + 1 - i;
    }
  }
  if (!pad) {
    *(start - 1) = 'b';
    *(start - 2) = '0';
    return {start - (needPrefix ? 2 : 0), buf + bitLens + 2};
  } else {
    return {buf + (needPrefix ? 0 : 2), buf + bitLens + 2};
  }
}

//获取整数的hex串
template <typename NumberType>
[[nodiscard]] std::string NumberToInternalHexStr(
    NumberType number, bool needPrefix = false,
    bool pad = false) requires std::is_integral_v<NumberType> {
  constexpr size_t hexLens = sizeof(NumberType) * 2;
  char buf[hexLens + 2] = {'0', 'x'};
  char* start = buf + hexLens + 1;
  for (size_t i = 0; i < hexLens; ++i) {
    auto value = (number >> (i * 4)) & 0b1111;
    assert(0 <= value && value <= 15);
    buf[hexLens + 1 - i] =
        static_cast<char>((value < 10 ? value + '0' : value - 10 + 'a'));
    if (!pad && value) {
      start = buf + hexLens + 1 - i;
    }
  }
  if (!pad) {
    *(start - 1) = 'x';
    *(start - 2) = '0';
    return {start - (needPrefix ? 2 : 0), buf + hexLens + 2};
  } else {
    return {buf + (needPrefix ? 0 : 2), buf + hexLens + 2};
  }
}

//把浮点数转换为字符串
//有可能出现 std::errc::value_to_large
//出错时返回空串
template <typename NumberType>
[[nodiscard]] std::string FloatToStr(
    NumberType number, int precision = 2,
    std::chars_format format = std::chars_format::fixed) requires
    std::is_floating_point_v<NumberType> {
  char buf[std::numeric_limits<NumberType>::digits10 + 16]{};
  auto [ptr, ec] =
      std::to_chars(buf, buf + sizeof buf, number, format, precision);
  if (ec != std::errc()) {
    assert(ec == std::errc::value_too_large);
    return "";
  }
  assert(ptr < buf + sizeof buf);
  return {buf, ptr};
}

//把指针按16进制输出
template <typename T>
[[nodiscard]] std::string PointerToStr(T* pointer, bool pad = true) {
  return NumberToInternalHexStr(reinterpret_cast<uintptr_t>(pointer), true,
                                true);
}

[[nodiscard]] std::string_view TrimToStringView(std::string_view str,
                                                char delim = ' ');
[[nodiscard]] std::string TrimToString(std::string_view str, char delim = ' ');

}  // namespace Cold::Base

#endif /* COLD_UTIL_STRINGUTIL */
