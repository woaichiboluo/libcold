#ifndef LOG_SINKS_BASICFILESINK
#define LOG_SINKS_BASICFILESINK

#include <cassert>
#include <chrono>
#include <cstdio>

#include "cold/log/LogFactory.h"
#include "cold/log/sinks/LogSink.h"

namespace Cold::Base {

class BasicFileSink : public LogSink {
 public:
  constexpr static std::chrono::seconds kFlushInterval =
      std::chrono::seconds(5);

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
      Base::ERROR("Open file error Reason: {}", ThisThread::ErrorMsg());
    }
  }

  void DoSink(const LogMessage& message) override {
    if (!fp_) {
      Base::ERROR("Bad Basic File Sink");
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
  Time lastFlushTime_;
};

inline std::shared_ptr<Logger> MakeBasicFileLogger(std::string loggerName,
                                                   std::string fileName,
                                                   bool append = false) {
  return LoggerFactory::MakeLogger<BasicFileSink>(std::move(loggerName),
                                                  std::move(fileName), append);
}

};  // namespace Cold::Base

#endif /* LOG_SINKS_BASICFILESINK */
