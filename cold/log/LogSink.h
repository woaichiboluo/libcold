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
  static constexpr const char* kDefaultLogSinkPattern =
      "%T %L <%N:%t> %c [%b:%l]%n";

  explicit LogSink() : formatter_(GetDefaultLogFormatter()) {}

  explicit LogSink(LogFormatterPtr formatter)
      : formatter_(std::move(formatter)) {}

  virtual ~LogSink() = default;

  LogSink(const LogSink&) = delete;
  LogSink& operator=(const LogSink&) = delete;

  void Sink(const LogMessage& message) {
    LockGuard guard(mutex_);
    assert(formatter_->Available());
    DoSink(message);
  }

  void Flush() {
    LockGuard guard(mutex_);
    DoFlush();
  }

  void SetFormatter(LogFormatterPtr formatter) {
    assert(formatter->Available());
    LockGuard guard(mutex_);
    formatter_ = std::move(formatter);
  }

  bool SetPattern(std::string_view pattern) {
    LockGuard guard(mutex_);
    assert(formatter_->Available());
    return formatter_->TryBuild(pattern);
  }

 protected:
  LogFormatterPtr formatter_;
  Mutex mutex_;

  virtual void DoSink(const LogMessage& message) = 0;
  virtual void DoFlush() = 0;

 private:
  static LogFormatterPtr GetDefaultLogFormatter() {
    auto formatter = std::make_unique<LogFormatter>(kDefaultLogSinkPattern);
    auto result = formatter->Build();
    assert(result);
    return formatter;
  }
};

};  // namespace Cold::Base

#endif /* COLD_LOG_LOGSINK */
