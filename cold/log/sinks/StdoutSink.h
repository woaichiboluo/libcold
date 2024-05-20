#ifndef LOG_SINKS_STDOUTSINK
#define LOG_SINKS_STDOUTSINK

#include <cstdio>
#include <memory>

#include "cold/log/LogFactory.h"
#include "cold/log/sinks/LogSink.h"

namespace Cold::Base {

class StdoutSink : public LogSink {
 public:
  StdoutSink() = default;
  explicit StdoutSink(LogFormatterPtr ptr) : LogSink(std::move(ptr)) {}
  ~StdoutSink() override = default;

 private:
  void DoSink(const LogMessage& message) override {
    LogBuffer buffer;
    formatter_->Format(message, buffer);
    fwrite(buffer.data(), sizeof(char), buffer.size(), stdout);
    fflush(stdout);
  }

  void DoFlush() override { fflush(stdout); }
};

inline std::shared_ptr<Logger> MakeStdoutLogger(std::string loggerName) {
  return LoggerFactory::MakeLogger<StdoutSink>(std::move(loggerName));
}

}  // namespace Cold::Base

#endif /* LOG_SINKS_STDOUTSINK */
