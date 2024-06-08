#ifndef NET_HTTP_DISPATCHERSERVLET
#define NET_HTTP_DISPATCHERSERVLET

#include <string_view>

#include "cold/net/http/Router.h"

namespace Cold::Net::Http {

class DispatcherServlet {
 public:
  DispatcherServlet() = default;
  virtual ~DispatcherServlet() = default;

  DispatcherServlet(const DispatcherServlet&) = delete;
  DispatcherServlet& operator=(const DispatcherServlet&) = delete;

  // true  找到合适的Servlet进行Dispathcer
  // false 没有找到合适的Servlet进行Dispatcher
  virtual bool DoDispathcer(std::string_view url, HttpRequest& request,
                            HttpResponse& response) = 0;
  virtual void AddServlet(std::string_view& url,
                          std::unique_ptr<HttpServlet> servlet) = 0;
  virtual void AddFilter(std::string_view& url,
                         std::unique_ptr<HttpFilter> servlet) = 0;
};

class DefaultDispatcherServlet : public DispatcherServlet {
 public:
  DefaultDispatcherServlet() : router_(std::make_unique<Router>()) {}

  ~DefaultDispatcherServlet() override = default;

  bool DoDispathcer(std::string_view url, HttpRequest& request,
                    HttpResponse& response) override {
    auto filters = router_->MatchFilters(url);
    for (auto& filter : filters) {
      if (!filter->DoFilter(request, response)) return true;
    }
    auto servlet = router_->MatchServlet(url);
    if (!servlet) return false;
    servlet->Handle(request, response);
    return true;
  }

  virtual void AddServlet(std::string_view& url,
                          std::unique_ptr<HttpServlet> servlet) override {
    router_->AddRoute(url, std::move(servlet));
  }

  virtual void AddFilter(std::string_view& url,
                         std::unique_ptr<HttpFilter> servlet) override {
    router_->AddRoute(url, std::move(servlet));
  }

 private:
  std::unique_ptr<Router> router_;
};

}  // namespace Cold::Net::Http

#endif /* NET_HTTP_DISPATCHERSERVLET */
