#ifndef LOG_SINKS_STDOUTCOLORSINK
#define LOG_SINKS_STDOUTCOLORSINK

#include <memory>

#include "cold/log/LogFactory.h"
#include "cold/log/sinks/LogSink.h"
#include "cold/util/ShellColor.h"

namespace Cold::Base {

class StdoutColorSink : public LogSink {
 public:
  static constexpr const char* kPattern = "%T %C%L%E <%N:%t> %c [%b:%l]%n";
  using FlagMap =
      std::unordered_map<char, std::unique_ptr<CustomFlagFormatter>>;

  StdoutColorSink() : LogSink(kPattern, GetFlagMap()) {}

  ~StdoutColorSink() override = default;

 private:
  struct ColorStartFlag : public CustomFlagFormatter {
    void Format(const LogMessage& message, LogBuffer& buffer) const override {
      constexpr static Color::ColorControl colors[6] = {
          Color::ColorControl(Color::FrontColor::kCyan),       // debug
          Color::ColorControl(Color::FrontColor::kBlue),       // trace
          Color::ColorControl(Color::FrontColor::kGreen),      // info
          Color::ColorControl(Color::FrontColor::kMagenta),    // warn
          Color::ColorControl(Color::FrontColor::kRed),        // error
          Color::ColorControl(Color::FrontColor::kBrightRed),  // fatal
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
      std::string_view view(Color::ColorControl::ColorEnd());
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

inline std::shared_ptr<Logger> MakeStdoutColorLogger(std::string loggerName) {
  return LoggerFactory::MakeLogger<StdoutColorSink>(std::move(loggerName));
}

}  // namespace Cold::Base

#endif /* LOG_SINKS_STDOUTCOLORSINK */
