#ifndef LOG_SINKS_STDOUTSINK
#define LOG_SINKS_STDOUTSINK

#include <cstdio>
#include <memory>

#include "cold/log/LogFormatter.h"
#include "cold/log/sinks/LogSink.h"

namespace Cold::Base {

class StdoutLogSink : public LogSink {
 public:
  constexpr static const char* kDefaultStdoutPattern = "";

  StdoutLogSink() = default;
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

}  // namespace Cold::Base

#endif /* LOG_SINKS_STDOUTSINK */
