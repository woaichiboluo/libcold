#ifndef COLD_BASE_LOGFORMATTER
#define COLD_BASE_LOGFORMATTER

#include <charconv>
#include <memory>
#include <source_location>
#include <string_view>

#include "cold/time/Time.h"
#include "third_party/fmt/include/fmt/format.h"

namespace Cold {

enum class LogLevel { TRACE, DEBUG, INFO, WARN, ERROR, FATAL, OFF };

struct LogMessage {
  LogLevel level;
  std::string_view threadId;
  std::string_view threadName;
  std::string_view loggerName;
  std::source_location location;
  std::string_view baseName;
  Time logTime;
  std::string_view logLine;
};

using LogBuffer = fmt::memory_buffer;

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

namespace detail {

class FlagFormatter {
 public:
  using FlagFormatterPtr = std::unique_ptr<FlagFormatter>;

  FlagFormatter() = default;
  virtual ~FlagFormatter() = default;

  FlagFormatter(const FlagFormatter&) = delete;
  FlagFormatter& operator=(const FlagFormatter&) = delete;

  virtual void Format(const LogMessage& message, LogBuffer& buffer) const = 0;
};

class ThreadIdFormatter : public FlagFormatter {
 public:
  ThreadIdFormatter() = default;
  ~ThreadIdFormatter() override = default;

  void Format(const LogMessage& message, LogBuffer& buffer) const override {
    buffer.append(message.threadId.begin(), message.threadId.end());
  }
};

class ThreadNameFormatter : public FlagFormatter {
 public:
  ThreadNameFormatter() = default;
  ~ThreadNameFormatter() override = default;
  void Format(const LogMessage& message, LogBuffer& buffer) const override {
    buffer.append(message.threadName.begin(), message.threadName.end());
  }
};

class FunctionNameFormatter : public FlagFormatter {
 public:
  FunctionNameFormatter() = default;
  ~FunctionNameFormatter() override = default;
  void Format(const LogMessage& message, LogBuffer& buffer) const override {
    std::string_view functionName(message.location.function_name());
    buffer.append(functionName.begin(), functionName.end());
  }
};

class FileNameFormatter : public FlagFormatter {
 public:
  FileNameFormatter() = default;
  ~FileNameFormatter() override = default;
  void Format(const LogMessage& message, LogBuffer& buffer) const override {
    std::string_view fileName(message.location.file_name());
    buffer.append(fileName.begin(), fileName.end());
  }
};

class BaseNameFormatter : public FlagFormatter {
 public:
  BaseNameFormatter() = default;
  ~BaseNameFormatter() override = default;
  void Format(const LogMessage& message, LogBuffer& buffer) const override {
    buffer.append(message.baseName.begin(), message.baseName.end());
  }
};

class FileLineFormatter : public FlagFormatter {
 public:
  FileLineFormatter() = default;
  ~FileLineFormatter() override = default;
  void Format(const LogMessage& message, LogBuffer& buffer) const override {
    char buf[64];
    auto [ptr, ec] =
        std::to_chars(buf, buf + sizeof buf, message.location.line());
    assert(ec == std::errc());
    buffer.append(buf, ptr);
  }
};

class LogLevelFormatter : public FlagFormatter {
 public:
  LogLevelFormatter() = default;
  ~LogLevelFormatter() override = default;
  constexpr static std::string_view kLogLevels[] = {"TRACE", "DEBUG", "INFO ",
                                                    "WARN ", "ERROR", "FATAL"};

  void Format(const LogMessage& message, LogBuffer& buffer) const override {
    auto level = static_cast<size_t>(message.level);
    buffer.append(kLogLevels[level].begin(), kLogLevels[level].end());
  }
};

class LoggerNameFormatter : public FlagFormatter {
 public:
  LoggerNameFormatter() = default;
  ~LoggerNameFormatter() override = default;

  void Format(const LogMessage& message, LogBuffer& buffer) const override {
    buffer.append(message.loggerName.begin(), message.loggerName.end());
  }
};

class LogLineFormatter : public FlagFormatter {
 public:
  LogLineFormatter() = default;
  ~LogLineFormatter() override = default;

  void Format(const LogMessage& message, LogBuffer& buffer) const override {
    buffer.append(message.logLine.begin(), message.logLine.end());
  }
};

class NewLineFormatter : public FlagFormatter {
 public:
  NewLineFormatter() = default;
  ~NewLineFormatter() override = default;
  constexpr static std::string_view kNewLine = "\n";
  void Format(const LogMessage& message, LogBuffer& buffer) const override {
    buffer.append(kNewLine.begin(), kNewLine.end());
  }
};

class PatternTextFormatter : public FlagFormatter {
 public:
  PatternTextFormatter(std::string text) : text_(std::move(text)) {}
  ~PatternTextFormatter() override = default;

  void Format(const LogMessage& message, LogBuffer& buffer) const override {
    buffer.append(text_.data(), text_.data() + text_.size());
  }

 private:
  std::string text_;
};

class LocalTimeFormatter : public FlagFormatter {
 public:
  LocalTimeFormatter() = default;
  ~LocalTimeFormatter() override = default;

  static void Pad2(char* begin, int v) {
    begin[0] = static_cast<char>(v / 10 + '0');
    begin[1] = static_cast<char>(v % 10 + '0');
  }

  static void Pad3(char* begin, int v) {
    begin[0] = static_cast<char>(v / 100 + '0');
    begin[1] = static_cast<char>(v / 10 % 10 + '0');
    begin[2] = static_cast<char>(v % 10 + '0');
  }

  static void Pad4(char* begin, int v) {
    begin[0] = static_cast<char>(v / 1000 + '0');
    Pad3(begin + 1, v % 1000);
  }

  void Format(const LogMessage& message, LogBuffer& buffer) const override {
    auto totalMs = message.logTime.TimeSinceEpochMilliSeconds();
    time_t seconds = totalMs / 1000;
    int milliseconds = static_cast<int>(totalMs - seconds * 1000);
    if (seconds != lastFormatSecond_) {
      lastFormatSecond_ = seconds;
      message.logTime.LocalExplode(&cachedTimeExploded_);
      Pad4(timeBuf_, cachedTimeExploded_.year);
      Pad2(timeBuf_ + 5, cachedTimeExploded_.month);
      Pad2(timeBuf_ + 8, cachedTimeExploded_.day);
      Pad2(timeBuf_ + 11, cachedTimeExploded_.hour);
      Pad2(timeBuf_ + 14, cachedTimeExploded_.minute);
      Pad2(timeBuf_ + 17, cachedTimeExploded_.second);
    }
    Pad3(timeBuf_ + 20, milliseconds);
    buffer.append(timeBuf_, timeBuf_ + 23);
  }

 private:
  mutable time_t lastFormatSecond_ = 0;
  mutable Time::TimeExploded cachedTimeExploded_;
  mutable char timeBuf_[32] = "xxxx-xx-xx oo:oo:oo.ooo";
};

}  // namespace detail

class CustomFlagFormatter : public detail::FlagFormatter {
 public:
  using CustomFlagFormatterPtr = std::unique_ptr<CustomFlagFormatter>;
  CustomFlagFormatter() = default;
  virtual ~CustomFlagFormatter() = default;
  virtual CustomFlagFormatterPtr Clone() const = 0;
};

class LogFormatter {
 public:
  using FlagFormatterPtr = std::unique_ptr<detail::FlagFormatter>;
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
    using namespace detail;
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
            std::make_unique<detail::PatternTextFormatter>(std::move(text)));
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
          std::make_unique<detail::PatternTextFormatter>(std::move(text)));
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

#endif /* COLD_BASE_LOGFORMATTER */
