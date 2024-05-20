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
  using FlagMap = std::unordered_map<char, CustomFlagFormatterPtr>;

  LogFormatter(std::string pattern, FlagMap flagMap = {});
  ~LogFormatter() = default;

  LogFormatter(const LogFormatter&) = delete;
  LogFormatter& operator=(const LogFormatter&) = delete;

  void Format(const LogMessage& message, LogBuffer& buffer) const {
    for (const auto& flag : formatSequence_) {
      flag->Format(message, buffer);
    }
  }

  const std::string& GetPattern() const { return pattern_; }

  void SetPattern(std::string_view pattern);

  LogFormatterPtr Clone() const;

 public:
  bool CompilePattern();
  void CompilePatternOrAbort();

  std::string pattern_;
  std::vector<FlagFormatterPtr> formatSequence_;
  FlagMap flagMap_;
};

}  // namespace Cold::Base

#endif /* COLD_LOG_LOGFORMATTER */
