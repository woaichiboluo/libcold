#ifndef COLD_LOG_LOGFACTORY
#define COLD_LOG_LOGFACTORY

#include <initializer_list>
#include <memory>

#include "cold/log/Logger.h"

namespace Cold::Base {

struct LoggerFactory {
  template <typename Sink, typename... SinkArgs>
  requires std::is_base_of_v<LogSink, Sink>
  static std::shared_ptr<Logger> MakeLogger(std::string loggerName,
                                            SinkArgs&&... args) {
    auto sink = std::make_shared<Sink>(std::forward<SinkArgs>(args)...);
    auto logger =
        std::make_shared<Logger>(std::move(loggerName), std::move(sink));
    LogManager::Instance().AddLogger(logger);
    return logger;
  }

  static std::shared_ptr<Logger> MakeLogger(
      std::string loggerName,
      std::initializer_list<std::shared_ptr<LogSink>>&& sinks) {
    auto logger =
        std::make_shared<Logger>(std::move(loggerName), std::move(sinks));
    LogManager::Instance().AddLogger(logger);
    return logger;
  }
};

}  // namespace Cold::Base

#endif /* COLD_LOG_LOGFACTORY */
