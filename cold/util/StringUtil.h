#ifndef COLD_UTIL_STRINGUTIL
#define COLD_UTIL_STRINGUTIL

#include <map>
#include <string>

namespace Cold {

inline void ParseKV(std::string_view kvStr, char kDelim, char kvDelim,
                    std::map<std::string, std::string>& m) {
  while (true) {
    auto keypos = kvStr.find(kDelim);
    if (keypos == kvStr.npos) return;
    auto valuepos = kvStr.find(kvDelim, keypos + 1);
    auto kv = kvStr.substr(0, valuepos);
    auto key = kv.substr(0, keypos);
    auto value = kv.substr(keypos + 1);
    m[std::string(key)] = value;
    if (valuepos == kvStr.npos) return;
    kvStr = kvStr.substr(valuepos + 1);
  }
}

inline std::map<std::string, std::string> ParseKV(std::string_view kvStr,
                                                  char kDelim, char kvDelim) {
  std::map<std::string, std::string> m;
  ParseKV(kvStr, kDelim, kvDelim, m);
  return m;
}

}  // namespace Cold

#endif /* COLD_UTIL_STRINGUTIL */
