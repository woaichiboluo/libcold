#ifndef COLD_UTIL_STRINGUTILS
#define COLD_UTIL_STRINGUTILS

#include <string_view>
#include <vector>

namespace Cold::Base {

inline std::string_view TrimString(std::string_view str, char c = ' ') {
  size_t begin = str.find_first_not_of(c);
  size_t end = str.find_last_not_of(c) + 1;
  return str.substr(begin, end - begin);
}

inline std::vector<std::string_view> SplitToViews(std::string_view str,
                                                  std::string_view delim,
                                                  bool skipEmpty = false) {
  std::vector<std::string_view> views;
  if (str.empty()) return views;
  for (size_t start = 0; start != std::string_view::npos;) {
    auto end = str.find(delim, start);
    std::string_view view;
    if (end == std::string_view::npos) {
      view = str.substr(start);
      start = end;
    } else {
      view = str.substr(start, end - start);
      start = end + 1;
    }
    if (skipEmpty && view.empty()) continue;
    views.push_back(view);
  }
  return views;
}

}  // namespace Cold::Base

#endif /* COLD_UTIL_STRINGUTILS */
