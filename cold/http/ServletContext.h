#ifndef COLD_HTTP_SERVLETCONTEXT
#define COLD_HTTP_SERVLETCONTEXT

#include <any>
#include <map>

#include "DefaultHttpServlet.h"
#include "DispatcherServlet.h"
#include "HttpSession.h"

namespace Cold::Http {

class ServletContext {
 public:
  ServletContext()
      : dispatcher_(std::make_unique<DefaultDisPatcherServlet>()),
        defaultServlet_(std::make_unique<DefaultHttpServlet>()) {}

  ~ServletContext() = default;

  ServletContext(const ServletContext&) = delete;
  ServletContext& operator=(const ServletContext&) = delete;

  void SetHost(std::string host) { host_ = std::move(host); }
  const std::string& GetHost() const { return host_; }

  void SetDispatcher(std::unique_ptr<DisPatcherServlet> dispatcher) {
    dispatcher_ = std::move(dispatcher);
  }
  DisPatcherServlet* GetDispatcher() { return dispatcher_.get(); }

  void SetDefaultServlet(std::unique_ptr<HttpServlet> servlet) {
    defaultServlet_ = std::move(servlet);
  }

  void ForwardTo(std::string_view url, HttpRequest& request,
                 HttpResponse& response) {
    bool useDefault = true;
    if (url == "/") {
      constexpr std::array<std::string_view, 3> kIndexPages = {
          "/", "/index.html", "/index"};
      for (auto& indexPage : kIndexPages) {
        if (dispatcher_->DoDispathcer(indexPage, request, response)) {
          useDefault = false;
          break;
        }
      }
    } else {
      useDefault = !dispatcher_->DoDispathcer(url, request, response);
    }
    if (useDefault && defaultServlet_) {
      defaultServlet_->DoService(request, response);
    }
  }

  ANY_MAP_CRUD(Attribute, attributes_)

  std::shared_ptr<HttpSession> CreateSession(HttpResponse& response) {
    auto session = sessionManager_.CreateSession(response);
    return session;
  }

  std::shared_ptr<HttpSession> GetSession(const std::string& sessionId) {
    return sessionManager_.GetSession(sessionId);
  }

 private:
  std::string host_;
  std::unique_ptr<DisPatcherServlet> dispatcher_;
  std::unique_ptr<HttpServlet> defaultServlet_;
  std::map<std::string, std::any> attributes_;
  HttpSessionManager sessionManager_;
};

}  // namespace Cold::Http

#endif /* COLD_HTTP_SERVLETCONTEXT */
