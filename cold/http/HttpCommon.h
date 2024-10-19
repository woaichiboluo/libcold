#ifndef COLD_HTTP_HTTPCOMMON
#define COLD_HTTP_HTTPCOMMON

#include <map>
#include <string>
#include <string_view>

#define MAP_CRUD(MethodName, Member)                                     \
  void Set##MethodName(std::string key, auto&& value) {                  \
    Member[std::move(key)] = std::move(value);                           \
  }                                                                      \
  bool Has##MethodName(const std::string& key) const {                   \
    return Member.contains(key);                                         \
  }                                                                      \
  std::string_view Get##MethodName(const std::string& key) const {       \
    auto it = Member.find(key);                                          \
    if (it == Member.end()) return "";                                   \
    return it->second;                                                   \
  }                                                                      \
  void Remove##MethodName(const std::string& key) { Member.erase(key); } \
  auto& GetAll##MethodName() { return Member; }                          \
  const auto& GetAll##MethodName() const { return Member; }

namespace Cold::Http {
static constexpr std::string_view kCRLF = "\r\n";
}  // namespace Cold::Http

#endif /* COLD_HTTP_HTTPCOMMON */
