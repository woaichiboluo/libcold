#ifndef COLD_LOG_LOGSINKS_NULLSINK
#define COLD_LOG_LOGSINKS_NULLSINK

#include "../LogFactory.h"
#include "LogSink.h"

namespace Cold {

class NullSink : public LogSink {
 public:
  NullSink() = default;
  explicit NullSink(LogFormatterPtr ptr) : LogSink(std::move(ptr)) {}
  ~NullSink() override = default;

 private:
  void DoSink(const LogMessage& message) override {}
  void DoFlush() override {}
};

inline std::shared_ptr<Logger> MakeNullLogger(std::string name) {
  return LogFactory::Create<NullSink>(std::move(name));
}

}  // namespace Cold

#endif /* COLD_LOG_LOGSINKS_NULLSINK */
