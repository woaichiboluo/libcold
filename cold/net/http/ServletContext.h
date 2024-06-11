#ifndef NET_HTTP_HTTPCONTEXT
#define NET_HTTP_HTTPCONTEXT

#include <map>
#include <memory>
#include <string>
#include <string_view>

#include "cold/net/http/DispatcherServlet.h"
#include "cold/thread/Lock.h"

namespace Cold::Net::Http {

class ServletContext {
 public:
  friend class HttpServer;

  ServletContext()
      : dispatcher_(std::make_unique<DefaultDispatcherServlet>()),
        defaultServlet_(std::make_unique<DefaultHttpServlet>()) {}

  ~ServletContext() = default;

  ServletContext(const ServletContext&) = delete;
  ServletContext& operator=(const ServletContext&) = delete;

  std::string_view GetHost() const { return host_; }

  bool HasAttribute(const std::string& key) const {
    Base::LockGuard guard(mutex_);
    return attributes_.contains(key);
  }

  std::string_view GetAttribute(const std::string& key) const {
    Base::LockGuard guard(mutex_);
    auto it = attributes_.find(key);
    if (it == attributes_.end()) return "";
    return it->second;
  }

  void SetAttribute(std::string key, std::string value) {
    Base::LockGuard guard(mutex_);
    attributes_[std::move(key)] = std::move(value);
  }

  void RemoveAttribute(const std::string& key) {
    Base::LockGuard guard(mutex_);
    attributes_.erase(key);
  }

  void ForwardTo(std::string_view url, HttpRequest& request,
                 HttpResponse& response) {
    if (!dispatcher_->DoDispathcer(url, request, response)) {
      defaultServlet_->Handle(request, response);
    }
  }

 private:
  std::string host_;
  mutable Base::Mutex mutex_;
  std::map<std::string, std::string> attributes_ GUARDED_BY(mutex_);
  std::unique_ptr<DispatcherServlet> dispatcher_;
  std::unique_ptr<HttpServlet> defaultServlet_;
};

}  // namespace Cold::Net::Http

#endif /* NET_HTTP_HTTPCONTEXT */
