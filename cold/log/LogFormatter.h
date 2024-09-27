#ifndef COLD_LOG_LOGFORMATTER
#define COLD_LOG_LOGFORMATTER

#include "../detail/FlagFormatter.h"

namespace Cold {

/*
Format flags:
  %t - thread id
  %N - thread name
  %f - function name
  %F - file name
  %b - base name
  %l - line number
  %L - log level
  %n - new line
  %T - local time
  %c - log line
  %m - logger name
  %<custom> - custom flag
  # custom extend with CustomFlagFormatter and implement Format method and Clone
*/

class LogFormatter {
 public:
  using FlagFormatterPtr = std::unique_ptr<Detail::FlagFormatter>;
  using CustomFlagFormatterPtr = std::unique_ptr<CustomFlagFormatter>;
  using LogFormatterPtr = std::unique_ptr<LogFormatter>;
  using FlagMap = std::unordered_map<char, CustomFlagFormatterPtr>;

  static constexpr const char* kDefaultPattern = "%T %L <%N:%t> %c [%b:%l]%n";

  LogFormatter(std::string pattern, FlagMap flagMap = {})
      : pattern_(std::move(pattern)), flagMap_(std::move(flagMap)) {
    CompilePatternOrAbort();
  }

  ~LogFormatter() = default;

  LogFormatter(const LogFormatter&) = delete;
  LogFormatter& operator=(const LogFormatter&) = delete;

  void Format(const LogMessage& message, LogBuffer& buffer) const {
    for (const auto& flag : formatSequence_) {
      flag->Format(message, buffer);
    }
  }

  const std::string& GetPattern() const { return pattern_; }

  void SetPattern(std::string_view pattern) {
    pattern_ = pattern;
    CompilePatternOrAbort();
  }

  LogFormatterPtr Clone() const {
    FlagMap flagMap;
    for (const auto& [key, value] : flagMap_) {
      flagMap[key] = value->Clone();
    }
    return std::make_unique<LogFormatter>(pattern_, std::move(flagMap));
  }

 public:
  static LogFormatterPtr MakeDefaultFormatter() {
    return std::make_unique<LogFormatter>(kDefaultPattern);
  }

  static bool HandleFlag(
      char c, std::vector<LogFormatter::FlagFormatterPtr>& sequence) {
    using namespace Detail;
    switch (c) {
      case 't':
        sequence.push_back(std::make_unique<ThreadIdFormatter>());
        return true;
      case 'N':
        sequence.push_back(std::make_unique<ThreadNameFormatter>());
        return true;
      case 'f':
        sequence.push_back(std::make_unique<FunctionNameFormatter>());
        return true;
      case 'F':
        sequence.push_back(std::make_unique<FileNameFormatter>());
        return true;
      case 'l':
        sequence.push_back(std::make_unique<FileLineFormatter>());
        return true;
      case 'L':
        sequence.push_back(std::make_unique<LogLevelFormatter>());
        return true;
      case 'n':
        sequence.push_back(std::make_unique<NewLineFormatter>());
        return true;
      case 'T':
        sequence.push_back(std::make_unique<LocalTimeFormatter>());
        return true;
      case 'c':
        sequence.push_back(std::make_unique<LogLineFormatter>());
        return true;
      case 'm':
        sequence.push_back(std::make_unique<LoggerNameFormatter>());
        return true;
      case 'b':
        sequence.push_back(std::make_unique<BaseNameFormatter>());
        return true;
      default:
        return false;
    }
  }

  bool CompilePattern() {
    std::vector<FlagFormatterPtr> sequence;
    auto n = pattern_.size();
    std::string text;
    for (size_t i = 0; i < n; ++i) {
      if (pattern_[i] == '%' && i + 1 >= n) return false;
      if (pattern_[i] != '%') {
        text.push_back(pattern_[i]);
        continue;
      }
      if (++i < n && pattern_[i] == '%') {
        text.push_back(pattern_[i]);
        continue;
      }
      if (!text.empty()) {
        sequence.push_back(
            std::make_unique<Detail::PatternTextFormatter>(std::move(text)));
      }
      auto it = flagMap_.find(pattern_[i]);
      if (it == flagMap_.end()) {  // use default flags
        if (!HandleFlag(pattern_[i], sequence)) return false;
      } else {  // use custom flags
        sequence.push_back(it->second->Clone());
      }
    }
    if (!text.empty()) {
      sequence.push_back(
          std::make_unique<Detail::PatternTextFormatter>(std::move(text)));
    }
    formatSequence_ = std::move(sequence);
    return true;
  }

  void CompilePatternOrAbort() {
    if (!CompilePattern()) {
      fprintf(stderr, "compile pattern error! error pattern: %s\n",
              pattern_.data());
      abort();
    }
  }

  std::string pattern_;
  std::vector<FlagFormatterPtr> formatSequence_;
  FlagMap flagMap_;
};

inline LogFormatter::LogFormatterPtr MakeLogFormatter(std::string pattern,
                                                      LogFormatter::FlagMap m) {
  return std::make_unique<LogFormatter>(std::move(pattern), std::move(m));
}

}  // namespace Cold

#endif /* COLD_LOG_LOGFORMATTER */
