#ifndef COLD_LOG_LOGSINKS_STDOUTCOLORSINK
#define COLD_LOG_LOGSINKS_STDOUTCOLORSINK

#include "../LogFactory.h"
#include "../ShellColor.h"
#include "LogSink.h"

namespace Cold {

class StdoutColorSink : public LogSink {
 public:
  static constexpr const char* kPattern = "%T %C%L%E <%N:%t> %c [%b:%l]%n";
  using FlagMap =
      std::unordered_map<char, std::unique_ptr<CustomFlagFormatter>>;

  StdoutColorSink() : LogSink(MakeLogFormatter(kPattern, GetFlagMap())) {}

  ~StdoutColorSink() override = default;

 private:
  struct ColorStartFlag : public CustomFlagFormatter {
    void Format(const LogMessage& message, LogBuffer& buffer) const override {
      constexpr static ShellColor::ColorControl colors[6] = {
          ShellColor::ColorControl(ShellColor::FrontColor::kCyan),     // debug
          ShellColor::ColorControl(ShellColor::FrontColor::kBlue),     // trace
          ShellColor::ColorControl(ShellColor::FrontColor::kGreen),    // info
          ShellColor::ColorControl(ShellColor::FrontColor::kMagenta),  // warn
          ShellColor::ColorControl(ShellColor::FrontColor::kRed),      // error
          ShellColor::ColorControl(
              ShellColor::FrontColor::kBrightRed),  // fatal
      };
      std::string_view view(
          colors[static_cast<size_t>(message.level)].ColorStart());
      buffer.append(view);
    }

    CustomFlagFormatterPtr Clone() const override {
      return std::make_unique<ColorStartFlag>();
    }
  };

  struct ColorEndFlag : public CustomFlagFormatter {
    void Format(const LogMessage& message, LogBuffer& buffer) const override {
      std::string_view view(ShellColor::ColorControl::ColorEnd());
      buffer.append(view);
    }

    CustomFlagFormatterPtr Clone() const override {
      return std::make_unique<ColorEndFlag>();
    }
  };

  static FlagMap GetFlagMap() {
    FlagMap flags;
    flags['C'] = std::make_unique<ColorStartFlag>();
    flags['E'] = std::make_unique<ColorEndFlag>();
    return flags;
  }

  void DoSink(const LogMessage& message) override {
    LogBuffer buffer;
    formatter_->Format(message, buffer);
    fwrite(buffer.data(), sizeof(char), buffer.size(), stdout);
    fflush(stdout);
  }

  void DoFlush() override { fflush(stdout); }
};

inline std::shared_ptr<Logger> MakeStdoutColorLogger(std::string name) {
  return LogFactory::Create<StdoutColorSink>(std::move(name));
}

}  // namespace Cold

#endif /* COLD_LOG_LOGSINKS_STDOUTCOLORSINK */
