#ifndef COLD_HTTP_DISPATCHERSERVLET
#define COLD_HTTP_DISPATCHERSERVLET

#include <memory>
#include <string_view>

#include "HttpFilter.h"
#include "HttpServlet.h"
#include "Router.h"

namespace Cold::Http {

class HttpRequest;
class HttpResponse;

class DisPatcherServlet {
 public:
  DisPatcherServlet() = default;
  virtual ~DisPatcherServlet() = default;

  DisPatcherServlet(const DisPatcherServlet&) = delete;
  DisPatcherServlet& operator=(const DisPatcherServlet&) = delete;

  virtual void AddServlet(std::string url,
                          std::unique_ptr<HttpServlet> servlet) = 0;

  virtual void AddFilter(std::string url,
                         std::unique_ptr<HttpFilter> servlet) = 0;

  // true find servlet handle the request
  // false not find servlet handle the request
  virtual bool DoDispathcer(std::string_view url, HttpRequest& request,
                            HttpResponse& response) = 0;
};

class DefaultDisPatcherServlet : public DisPatcherServlet {
 public:
  DefaultDisPatcherServlet() : router_() {}
  ~DefaultDisPatcherServlet() override = default;

  void AddServlet(std::string url,
                  std::unique_ptr<HttpServlet> servlet) override {
    router_.AddServlet(std::move(url), std::move(servlet));
  }

  void AddFilter(std::string url, std::unique_ptr<HttpFilter> filter) override {
    router_.AddFilter(std::move(url), std::move(filter));
  }

  bool DoDispathcer(std::string_view url, HttpRequest& request,
                    HttpResponse& response) override {
    auto filterChain = router_.MatchFilterChain(url);
    for (auto& filter : filterChain) {
      if (filter->DoFilter(request, response)) {
        return true;
      }
    }
    auto servlet = router_.MatchServlet(url);
    if (servlet) {
      servlet->DoService(request, response);
      return true;
    }
    return false;
  }

 private:
  Router router_;
};

}  // namespace Cold::Http

#endif /* COLD_HTTP_DISPATCHERSERVLET */
