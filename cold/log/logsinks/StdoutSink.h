#ifndef COLD_LOG_LOGSINKS_STDOUTSINK
#define COLD_LOG_LOGSINKS_STDOUTSINK

#include <cstdio>

#include "../LogFactory.h"
#include "LogSink.h"

namespace Cold {

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

inline std::shared_ptr<Logger> MakeStdoutLogger(std::string name) {
  return LogFactory::Create<StdoutSink>(std::move(name));
}

}  // namespace Cold

#endif /* COLD_LOG_LOGSINKS_STDOUTSINK */
