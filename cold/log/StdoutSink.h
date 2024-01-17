#ifndef COLD_LOG_STDOUTSINK
#define COLD_LOG_STDOUTSINK

#include <cstdio>
#include <memory>

#include "cold/log/LogFormatter.h"
#include "cold/log/LogSink.h"
#include "cold/util/ShellColor.h"

namespace Cold::Base {

class StdoutLogSink : public LogSink {
 public:
  constexpr static const char* kDefaultStdoutPattern = "";

  StdoutLogSink() {}
  explicit StdoutLogSink(LogFormatterPtr ptr) : LogSink(std::move(ptr)) {}
  ~StdoutLogSink() override = default;

 private:
  void DoSink(const LogMessage& message) override {
    LogBuffer buffer;
    formatter_->Format(message, buffer);
    fwrite(buffer.data(), sizeof(char), buffer.size(), stdout);
    fflush(stdout);
  }

  void DoFlush() override { fflush(stdout); }
};

// class DefaultLogLevelColor : public CustomFlagFormatter {};

class StdoutColorLogSink : public LogSink {
 public:
  constexpr static const char* kStdoutColorSinkPattern =
      "%T %C%L%K <%N:%t>  %c [%b:%l]%n";

  StdoutColorLogSink() {
    auto formatter = std::make_unique<LogFormatter>();
    formatter->AddFlag('C', std::make_unique<CustomColorFlag>());
    formatter->AddFlag('K', std::make_unique<ColorResetFlag>());
    formatter->SetPattern(kStdoutColorSinkPattern);
    SetFormatter(std::move(formatter));
  }

  explicit StdoutColorLogSink(LogFormatterPtr ptr) : LogSink(std::move(ptr)) {}
  ~StdoutColorLogSink() override = default;

 private:
  class CustomColorFlag : public CustomFlagFormatter {
   public:
    void Format(const LogMessage& message, LogBuffer& buffer) const override {
      using namespace Color;
      constexpr static TerminalColorPrefix colors[6] = {
          TerminalColorPrefix(TerminalTextColor::cyan),      // debug
          TerminalColorPrefix(TerminalTextColor::blue),      // trace
          TerminalColorPrefix(TerminalTextColor::green),     // info
          TerminalColorPrefix(TerminalTextColor::magenta),   // warn
          TerminalColorPrefix(TerminalTextColor::red),       // error
          TerminalColorPrefix(TerminalTextColor::lightRed),  // fatal
      };
      std::string_view prefix =
          colors[static_cast<size_t>(message.level)].data();
      buffer.append(prefix.begin(), prefix.end());
    }
    CustomFlagFormatterPtr Clone() const override {
      return std::make_unique<CustomColorFlag>();
    }
  };

  class ColorResetFlag : public CustomFlagFormatter {
   public:
    void Format(const LogMessage& message, LogBuffer& buffer) const override {
      buffer.append(Color::ResetColor.begin(), Color::ResetColor.end());
    }
    CustomFlagFormatterPtr Clone() const override {
      return std::make_unique<ColorResetFlag>();
    }
  };

  void DoSink(const LogMessage& message) override {
    LogBuffer buffer;
    formatter_->Format(message, buffer);
    fwrite(buffer.data(), sizeof(char), buffer.size(), stdout);
    fflush(stdout);
  }

  void DoFlush() override { fflush(stdout); }
};

}  // namespace Cold::Base

#endif /* COLD_LOG_STDOUTSINK */
