#ifndef COLD_LOG_BASICFILESINK
#define COLD_LOG_BASICFILESINK

#include <cassert>
#include <chrono>
#include <cstdio>

#include "cold/log/LogFormatter.h"
#include "cold/log/LogSink.h"
#include "cold/log/Logger.h"

namespace Cold::Base {

class BasicFileSink : public LogSink {
 public:
  constexpr static std::chrono::seconds kFlushInterval =
      std::chrono::seconds(15);

  explicit BasicFileSink(std::string fileName, bool append = false)
      : fileName_(std::move(fileName)) {
    OpenFile(append ? "ab" : "wb");
  }

  BasicFileSink(LogFormatterPtr formatter, std::string fileName,
                bool append = false)
      : LogSink(std::move(formatter)), fileName_(std::move(fileName)) {
    OpenFile(append ? "ab" : "wb");
  }

  ~BasicFileSink() override {
    if (fp_) fclose(fp_);
  }

 private:
  void OpenFile(const char* mode) {
    fp_ = fopen(fileName_.data(), mode);
    if (!fp_) {
      LOG_ERROR(GetMainLogger(), "Open file error Reason:{}",
                ThisThread::ErrorMsg());
    }
  }

  void DoSink(const LogMessage& message) override {
    if (!fp_) {
      LOG_ERROR(GetMainLogger(), "Bad Basic File Sink");
      return;
    }

    LogBuffer buffer;
    formatter_->Format(message, buffer);

    fwrite_unlocked(buffer.data(), sizeof(char), buffer.size(), fp_);

    auto elapsed = message.logTime - lastFlushTime_;
    if (elapsed > kFlushInterval) {
      DoFlush();
      lastFlushTime_ = message.logTime;
    }
  }

  void DoFlush() override { fflush(fp_); }

  const std::string& GetFileName() { return fileName_; }

 private:
  std::string fileName_;
  FILE* fp_ = nullptr;
  Time lastFlushTime_ = Time::Now();
};

};  // namespace Cold::Base

#endif /* COLD_LOG_BASICFILESINK */
