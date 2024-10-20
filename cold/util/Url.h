#ifndef COLD_UTIL_URL
#define COLD_UTIL_URL

#include <map>
#include <string>

#include "../detail/fmt.h"

namespace Cold {

inline std::string UrlEncode(std::string_view url) {
  static constexpr const std::string_view k_HEX = "0123456789ABCDEF";
  std::string ret;
  ret.reserve(url.size());
  for (const auto& c : url) {
    if (isalpha(c) || isdigit(c))
      ret.push_back(c);
    else {
      ret.push_back('%');
      ret.push_back(k_HEX[static_cast<uint8_t>(c) >> 4]);
      ret.push_back(k_HEX[c & 0xF]);
    }
  }
  return ret;
}

inline std::string UrlDecode(std::string_view url) {
  constexpr static auto HexToNum = [](char c) -> uint8_t {
    auto h = toupper(c);
    return static_cast<uint8_t>('A' <= h && h <= 'Z' ? h - 'A' + 10 : h - '0');
  };
  std::string ret;
  for (size_t i = 0; i < url.size(); ++i) {
    if (url[i] == '+') {
      ret.push_back(' ');
    } else if (url[i] == '%' && i + 2 < url.size() && isxdigit(url[i + 1]) &&
               isxdigit(url[i + 2])) {
      ret.push_back(static_cast<char>((HexToNum(url[i + 1]) << 4) |
                                      HexToNum(url[i + 2])));
      i += 2;
    } else {
      ret.push_back(url[i]);
    }
  }
  return ret;
}

// return {url, query, fragment}
inline std::tuple<std::string, std::string, std::string> DecodeHttpRequestUrl(
    std::string_view urlencoded) {
  std::string url = UrlDecode(urlencoded), query, fragment;
  std::string_view view(url);
  auto pos = view.find_last_of('#');
  if (pos != std::string_view::npos) {
    fragment = view.substr(pos + 1);
    view = view.substr(0, pos);
  }
  pos = view.find('?');
  if (pos != std::string_view::npos) {
    query = view.substr(pos + 1);
    url = url.substr(0, pos);
  }
  return {url, query, fragment};
}

inline std::string EncodeHttpRequestUrl(std::string_view url,
                                        std::string_view query,
                                        std::string_view fragment = "") {
  if (fragment.empty()) {
    return UrlEncode(fmt::format("{}?{}", url, query));
  } else {
    return UrlEncode(fmt::format("{}?{}#{}", url, query, fragment));
  }
}

inline std::string EncodeHttpRequestUrl(
    std::string_view url, std::map<std::string, std::string> query,
    std::string_view fragment = "") {
  std::string queryStr;
  auto begin = query.begin();
  for (size_t i = 0; i < query.size(); ++i) {
    queryStr += begin->first + "=" + begin->second;
    if (i + 1 != query.size()) {
      queryStr += "&";
    }
    ++begin;
  }
  return EncodeHttpRequestUrl(url, queryStr, fragment);
}

}  // namespace Cold

#endif /* COLD_UTIL_URL */
