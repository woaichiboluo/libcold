#ifndef COLD_LOG_LOGSINKFACTORY
#define COLD_LOG_LOGSINKFACTORY

#include <strings.h>

#include <cassert>
#include <map>
#include <variant>

#include "cold/log/sinks/BasicFileSink.h"
#include "cold/log/sinks/NullSink.h"
#include "cold/log/sinks/StdoutColorSink.h"
#include "cold/log/sinks/StdoutSink.h"

namespace Cold::Base {

class LogSinkFactory {
 public:
  using LogSinkPtr = std::shared_ptr<LogSink>;
  using ArgList = std::vector<std::variant<int, bool, std::string>>;
  using Func = std::function<LogSinkPtr(const ArgList&)>;

  ~LogSinkFactory() = default;

  LogSinkFactory(const LogSinkFactory&) = delete;
  LogSinkFactory& operator=(const LogSinkFactory&) = delete;

  static LogSinkPtr MakeSink(const std::string& type,
                             const std::string& pattern = "",
                             const ArgList& args = {}) {
    auto& facotry = Instance();
    assert(facotry.maker_.contains(type));
    auto ret = facotry.maker_[type](args);
    if (!pattern.empty()) ret->SetPattern(pattern);
    return ret;
  }

  template <typename Sink, typename... SinkArgs>
  static LogSinkPtr MakeSink(SinkArgs&&... args) {
    return std::make_shared<Sink>(std::forward<SinkArgs>(args)...);
  }

 private:
  LogSinkFactory() : maker_(IgnoreCaseCompare) {
    maker_.emplace("nullsink", MakeNullSink);
    maker_.emplace("stdoutsink", MakeStdoutSink);
    maker_.emplace("basicfilesink", MakeBasicFileSink);
    maker_.emplace("stdoutcolorsink", MakeStdoutColorSink);
  }

  static bool IgnoreCaseCompare(const std::string& a, const std::string& b) {
    return strcasecmp(a.data(), b.data()) < 0;
  }

  static LogSinkFactory& Instance() {
    static LogSinkFactory f;
    return f;
  }

  static SinkPtr MakeNullSink(const ArgList& args) {
    return std::make_shared<NullLogSink>();
  }

  static SinkPtr MakeStdoutSink(const ArgList& args) {
    return std::make_shared<StdoutSink>();
  }

  static SinkPtr MakeBasicFileSink(const ArgList& args) {
    if (args.size() == 1)
      return std::make_shared<BasicFileSink>(std::get<std::string>(args[0]));
    else
      return std::make_shared<BasicFileSink>(std::get<std::string>(args[0]),
                                             std::get<bool>(args[1]));
  }

  static SinkPtr MakeStdoutColorSink(const ArgList& args) {
    return std::make_shared<StdoutColorSink>();
  }

  std::map<std::string, Func, decltype(IgnoreCaseCompare)*> maker_;
};

}  // namespace Cold::Base

#endif /* COLD_LOG_LOGSINKFACTORY */
