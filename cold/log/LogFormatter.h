#ifndef COLD_LOG_LOGFORMATTER
#define COLD_LOG_LOGFORMATTER

#include <memory>
#include <unordered_map>
#include <vector>

#include "cold/log/LogCommon.h"

namespace Cold::Base {
class FlagFormatter {
 public:
  using FlagFormatterPtr = std::unique_ptr<FlagFormatter>;

  FlagFormatter() = default;
  virtual ~FlagFormatter() = default;

  FlagFormatter(const FlagFormatter&) = delete;
  FlagFormatter& operator=(const FlagFormatter&) = delete;

  virtual void Format(const LogMessage& message, LogBuffer& buffer) const = 0;
};

class CustomFlagFormatter : public FlagFormatter {
 public:
  using CustomFlagFormatterPtr = std::unique_ptr<CustomFlagFormatter>;
  CustomFlagFormatter() = default;
  virtual ~CustomFlagFormatter() = default;
  virtual CustomFlagFormatterPtr Clone() const = 0;
};

class LogFormatter {
 public:
  using FlagFormatterPtr = std::unique_ptr<FlagFormatter>;
  using CustomFlagFormatterPtr = std::unique_ptr<CustomFlagFormatter>;
  using LogFormatterPtr = std::unique_ptr<LogFormatter>;

  LogFormatter(std::string pattern = "")
      : pattern_(std::move(pattern)), available_(false) {
    available_ = CompilePattern();
  }
  ~LogFormatter() = default;

  LogFormatter(const LogFormatter&) = delete;
  LogFormatter& operator=(const LogFormatter&) = delete;

  void Format(const LogMessage& message, LogBuffer& buffer) {
    for (const auto& flag : formatSequence_) {
      flag->Format(message, buffer);
    }
  }

  LogFormatter& AddFlag(char flag, CustomFlagFormatterPtr ptr) {
    flagMap_[flag] = std::move(ptr);
    return *this;
  }

  const std::string& GetPattern() const { return pattern_; }

  void SetPattern(std::string_view pattern) {
    pattern_ = pattern;
    available_ = CompilePattern();
  }

  bool Available() const { return available_; }

  LogFormatterPtr Clone() const;

 private:
  bool CompilePattern();

  std::string pattern_;
  bool available_;
  std::vector<FlagFormatterPtr> formatSequence_;
  std::unordered_map<char, CustomFlagFormatterPtr> flagMap_;
};

}  // namespace Cold::Base

#endif /* COLD_LOG_LOGFORMATTER */
