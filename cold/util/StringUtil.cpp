#include "cold/util/StringUtil.h"

using namespace Cold;

std::vector<std::string_view> Base::SplitToStringView(
    std::string_view content, std::string_view separator) {
  return Base::internal::Split<std::string_view>(content, separator);
}

std::vector<std::string> Base::SplitToString(std::string_view content,
                                             std::string_view separator) {
  return Base::internal::Split<std::string>(content, separator);
}

std::string_view Base::TrimToStringView(std::string_view str, char delim) {
  auto front = str.find_first_not_of(delim);
  if (front == std::string_view::npos) return "";
  auto back = str.find_last_not_of(delim);
  return {str.data() + front, str.data() + back + 1};
}

std::string Base::TrimToString(std::string_view str, char delim) {
  return std::string(TrimToStringView(str, delim));
}