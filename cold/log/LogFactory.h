#ifndef COLD_LOG_LOGFACTORY
#define COLD_LOG_LOGFACTORY

#include "../log/LogManager.h"

namespace Cold {

class LogSink;

struct LogSinkFactory {
  template <typename T, typename... Args>
  static std::shared_ptr<LogSink> Create(Args &&...args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
  }
};

struct LogFactory {
  static std::shared_ptr<Logger> MakeLogger(
      std::string name,
      std::initializer_list<std::shared_ptr<LogSink>> &&list) {
    return MakeLogger(std::move(name), list.begin(), list.end());
  }

  template <typename It>
  static std::shared_ptr<Logger> MakeLogger(std::string name, It begin,
                                            It end) {
    auto logger = std::make_shared<Logger>(std::move(name), begin, end);
    LogManager::GetInstance().Add(logger);
    return logger;
  }

  template <typename T, typename... Args>
  static std::shared_ptr<Logger> Create(std::string name, Args &&...args) {
    auto sink = LogSinkFactory::Create<T>(std::forward<Args>(args)...);
    return MakeLogger(std::move(name), {sink});
  }
};

}  // namespace Cold

#endif /* COLD_LOG_LOGFACTORY */
