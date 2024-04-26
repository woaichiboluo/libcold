#ifndef COLD_LOG_NULLSINK
#define COLD_LOG_NULLSINK

#include "cold/log/LogCommon.h"
#include "cold/log/sinks/LogSink.h"

namespace Cold::Base {

class NullLogSink : public LogSink {
 public:
  NullLogSink() {}
  explicit NullLogSink(LogFormatterPtr ptr) : LogSink(std::move(ptr)) {}
  ~NullLogSink() override = default;

 private:
  void DoSink(const LogMessage& message) override {}
  void DoFlush() override {}
};

};  // namespace Cold::Base

#endif /* COLD_LOG_NULLSINK */
