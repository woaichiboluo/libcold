#ifndef COLD_DETAIL_FLAGFORMATTER
#define COLD_DETAIL_FLAGFORMATTER

#include <charconv>
#include <memory>

#include "LogCommon.h"

namespace Cold {

namespace Detail {
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
}  // namespace Detail

class CustomFlagFormatter : public Detail::FlagFormatter {
 public:
  using CustomFlagFormatterPtr = std::unique_ptr<CustomFlagFormatter>;
  CustomFlagFormatter() = default;
  virtual ~CustomFlagFormatter() = default;
  virtual CustomFlagFormatterPtr Clone() const = 0;
};

}  // namespace Cold

#endif /* COLD_DETAIL_FLAGFORMATTER */
