#ifndef COLD_UTIL_CONFIG
#define COLD_UTIL_CONFIG

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <mutex>

#include "third_party/nlohmann/json.hpp"

namespace Cold::Base {

// key use the json_pointer
// must start with /

class Config {
 public:
  using List = std::initializer_list<std::string_view>;
  Config() = default;

  Config(std::string filePath) { LoadConfig(filePath); }

  ~Config() = default;

  static Config& GetGloablDefaultConfig() {
    static Config config;
    static std::once_flag flag;
    std::call_once(flag, [&]() {
      using nlohmann::literals::operator""_json;
      const char* p = getenv("COLD_CONFIG_PATH");
      if (p == nullptr) p = "config.json";
      if (!std::filesystem::exists(p)) {
        config.configJson_ = R"({
         "logs": {
           "name": "main-logger",
           "level": "info",
           "flush-level": "error"
         },
         "http": {
           "read-timeout-ms": 15000,
           "sendfile-timeout-ms": 15000,
           "write-timeout-ms": 15000,
           "max-url-size": 190000,
           "max-header-field-size": 1024,
           "max-header-value-size": 10240,
           "max-headers-count": 100,
           "max-body-size": 1048576
         }
        })"_json;
        config.configFilePath_ = p;
        config.SaveConfig();
      } else {
        config.LoadConfig(p);
      }
    });
    return config;
  }

  bool Contains(const std::string& key) const {
    return configJson_.contains(nlohmann::json::json_pointer(key));
  }

  auto GetConfig(const std::string& key) const {
    return configJson_[nlohmann::json::json_pointer(key)];
  }

  // when not found will set config
  template <typename T>
  T GetOrDefault(const std::string& key, const T& value) {
    if (Contains(key)) return GetConfig(key);
    SetConfig(key, value);
    return value;
  }

  template <typename T>
  void SetConfig(const std::string& key, const T& value) {
    configJson_[nlohmann::json::json_pointer(key)] = value;
  }

  nlohmann::json GetConfigJson() const { return configJson_; }

  std::string DumpConfig() const { return configJson_.dump(4); }

  bool LoadConfig(std::string filePath) {
    std::ifstream in(filePath);
    if (!in.is_open()) return false;
    std::string buf{std::istreambuf_iterator<char>(in),
                    std::istreambuf_iterator<char>()};
    try {
      configJson_ = nlohmann::json::parse(buf);
      configFilePath_ = std::move(filePath);
    } catch (const nlohmann::json::parse_error& e) {
      return false;
    }
    return true;
  }

  bool SaveConfig() {
    std::ofstream out(configFilePath_);
    if (!out.is_open()) return false;
    out << std::setw(4) << configJson_;
    return true;
  }

 private:
  std::string configFilePath_;
  nlohmann::json configJson_;
};

}  // namespace Cold::Base

#endif /* COLD_UTIL_CONFIG */
