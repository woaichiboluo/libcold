#ifndef COLD_LOG_LOGSINKS_LOGSINK
#define COLD_LOG_LOGSINKS_LOGSINK

#include <mutex>

#include "../LogFormatter.h"

namespace Cold {

class LogSink {
 public:
  using LogFormatterPtr = std::unique_ptr<LogFormatter>;

  LogSink() : formatter_(LogFormatter::MakeDefaultFormatter()) {}

  explicit LogSink(LogFormatterPtr formatter)
      : formatter_(std::move(formatter)) {}

  virtual ~LogSink() = default;

  LogSink(const LogSink&) = delete;
  LogSink& operator=(const LogSink&) = delete;

  void Sink(const LogMessage& message) {
    std::unique_lock guard(mutex_);
    DoSink(message);
  }

  void Flush() {
    std::unique_lock guard(mutex_);
    DoFlush();
  }

  void SetFormatter(LogFormatterPtr formatter) {
    std::unique_lock guard(mutex_);
    formatter_ = std::move(formatter);
  }

  void SetPattern(std::string_view pattern) {
    std::unique_lock guard(mutex_);
    formatter_->SetPattern(pattern);
  }

 protected:
  std::mutex mutex_;
  LogFormatterPtr formatter_;

  virtual void DoSink(const LogMessage& message) = 0;
  virtual void DoFlush() = 0;
};

using SinkPtr = std::shared_ptr<LogSink>;

}  // namespace Cold

#endif /* COLD_LOG_LOGSINKS_LOGSINK */
