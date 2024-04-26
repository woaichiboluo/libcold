#ifndef COLD_LOG_LOGSINK
#define COLD_LOG_LOGSINK

#include <memory>

#include "cold/log/LogCommon.h"
#include "cold/log/LogFormatter.h"
#include "cold/thread/Lock.h"

namespace Cold::Base {

class LogSink {
 public:
  using LogFormatterPtr = std::unique_ptr<LogFormatter>;

  explicit LogSink() : formatter_(std::make_unique<LogFormatter>()) {}

  explicit LogSink(LogFormatterPtr formatter)
      : formatter_(std::move(formatter)) {}

  virtual ~LogSink() = default;

  LogSink(const LogSink&) = delete;
  LogSink& operator=(const LogSink&) = delete;

  void Sink(const LogMessage& message) {
    LockGuard guard(mutex_);
    DoSink(message);
  }

  void Flush() {
    LockGuard guard(mutex_);
    DoFlush();
  }

  void SetFormatter(LogFormatterPtr formatter) {
    LockGuard guard(mutex_);
    formatter_ = std::move(formatter);
  }

  void SetPattern(std::string_view pattern) {
    LockGuard guard(mutex_);
    formatter_->SetPattern(pattern);
  }

 protected:
  Mutex mutex_;
  LogFormatterPtr formatter_;

  virtual void DoSink(const LogMessage& message) = 0;
  virtual void DoFlush() = 0;
};

using SinkPtr = std::shared_ptr<LogSink>;

};  // namespace Cold::Base

#endif /* COLD_LOG_LOGSINK */
