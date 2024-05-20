#ifndef COLD_LOG_LOGFACTORY
#define COLD_LOG_LOGFACTORY

#include <memory>

#include "cold/log/Logger.h"

namespace Cold::Base {

struct LoggerFactory {
  template <typename Sink, typename... SinkArgs>
  static std::shared_ptr<Logger> MakeLogger(std::string loggerName,
                                            SinkArgs&&... args) {
    auto sink = std::make_shared<Sink>(std::forward<SinkArgs>(args)...);
    auto logger =
        std::make_shared<Logger>(std::move(loggerName), std::move(sink));
    LogManager::Instance().AddLogger(logger);
    return logger;
  }
};

}  // namespace Cold::Base

#endif /* COLD_LOG_LOGFACTORY */
