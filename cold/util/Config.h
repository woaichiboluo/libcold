#ifndef COLD_UTIL_CONFIG
#define COLD_UTIL_CONFIG

#include <fstream>

#include "../third_party/nlohmann/json.hpp"

namespace Cold {

class Config {
 public:
  Config() = default;
  explicit Config(std::string_view filePath) { LoadConfig(filePath); }
  ~Config() = default;

  bool LoadConfig(std::string_view filePath) {
    configFilePath_ = filePath;
    std::ifstream in(configFilePath_);
    if (!in.is_open()) return false;
    std::string buf{std::istreambuf_iterator<char>(in),
                    std::istreambuf_iterator<char>()};
    try {
      configJson_ = nlohmann::json::parse(buf);
    } catch (...) {
      return false;
    }
    return true;
  }

  bool SaveConfig() {
    std::ofstream out(configFilePath_);
    if (!out.is_open()) return false;
    out << configJson_.dump(4);
    return true;
  }

  bool Contains(const std::string& key) const {
    return configJson_.contains(nlohmann::json::json_pointer(key));
  }

  auto GetConfig(const std::string& key) const {
    return configJson_[nlohmann::json::json_pointer(key)];
  }

  template <typename T>
  T GetOrDefault(const std::string& key, const T& value,
                 bool notExistSave = true) {
    if (Contains(key)) return GetConfig(key);
    SetConfig(key, value);
    if (notExistSave) SaveConfig();
    return value;
  }

  template <typename T>
  void SetConfig(const std::string& key, const T& value) {
    configJson_[nlohmann::json::json_pointer(key)] = value;
  }

  nlohmann::json GetConfigJson() const { return configJson_; }
  nlohmann::json& GetMutableConfigJson() { return configJson_; }

  std::string Dump() const { return configJson_.dump(4); }

 private:
  std::string configFilePath_;
  nlohmann::json configJson_;
};

class GlobalConfig {
 public:
  GlobalConfig(const GlobalConfig&) = delete;
  GlobalConfig& operator=(const GlobalConfig&) = delete;

  static Config& GetInstance() {
    static GlobalConfig instance;
    return instance.config_;
  }

  template <typename T>
  static T GetOrDefault(const std::string& key, const T& value,
                        bool notExistSave = true) {
    auto& config = GetInstance();
    auto ret = config.GetOrDefault(key, value);
    return ret;
  }

 private:
  GlobalConfig() {
    const char* p = getenv("COLD_CONFIG_PATH");
    if (p == nullptr) p = "config.json";
    config_.LoadConfig(p);
  }

  ~GlobalConfig() = default;

  Config config_;
};

}  // namespace Cold

#endif /* COLD_UTIL_CONFIG */
