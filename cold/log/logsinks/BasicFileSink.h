#ifndef COLD_LOG_LOGSINKS_BASICFILESINK
#define COLD_LOG_LOGSINKS_BASICFILESINK

#include <cassert>
#include <chrono>
#include <cstdio>

#include "cold/log/LogFactory.h"
#include "cold/log/logsinks/LogSink.h"

namespace Cold {

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
      fprintf(stderr, "Open file error.");
    }
  }

  void DoSink(const LogMessage& message) override {
    if (!fp_) {
      fprintf(stderr, "FileSink file pointer is null.");
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

inline std::shared_ptr<Logger> MakeFileLogger(std::string name,
                                              std::string fileName,
                                              bool append = false) {
  return LogFactory::Create<BasicFileSink>(std::move(name), std::move(fileName),
                                           append);
}

};  // namespace Cold

#endif /* COLD_LOG_LOGSINKS_BASICFILESINK */
