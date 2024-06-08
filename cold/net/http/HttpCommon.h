#ifndef NET_HTTP_HTTPCOMMON
#define NET_HTTP_HTTPCOMMON

#include <cctype>
#include <cstdint>
#include <string>

namespace Cold::Net::Http {

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

}  // namespace Cold::Net::Http

#endif /* NET_HTTP_HTTPCOMMON */
