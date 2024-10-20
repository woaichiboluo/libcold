#ifndef COLD_HTTP_HTTPSESSION
#define COLD_HTTP_HTTPSESSION

#include <any>
#include <mutex>

#include "HttpResponse.h"

namespace Cold::Http {

class HttpSessionManager;

class HttpSession {
 public:
  HttpSession(std::string sessionId, HttpSessionManager* manager)
      : sessionId_(std::move(sessionId)), manager_(manager) {}
  ~HttpSession() = default;

  HttpSession(const HttpSession&) = delete;
  HttpSession& operator=(const HttpSession&) = delete;

  const std::string& GetSessionId() const { return sessionId_; }

  void Invalidate();

  ANY_MAP_CRUD(Attribute, attributes_)

 private:
  std::string sessionId_;
  HttpSessionManager* manager_;

  std::map<std::string, std::any> attributes_;
};

class HttpSessionManager {
 public:
  using SessionPtr = std::shared_ptr<HttpSession>;
  HttpSessionManager() = default;
  ~HttpSessionManager() = default;

  HttpSessionManager(const HttpSessionManager&) = delete;
  HttpSessionManager& operator=(const HttpSessionManager&) = delete;

  SessionPtr CreateSession(HttpResponse& response) {
    auto uuid = GenerateUUID();
    SessionPtr ptr;
    {
      std::lock_guard guard(mutex_);
      ptr = std::make_shared<HttpSession>(uuid, this);
      sessions_[uuid] = ptr;
    }
    HttpCookie cookie;
    cookie.key = "SessionID";
    cookie.value = std::move(uuid);
    cookie.maxAge = 60 * 60;
    cookie.path = "/";
    response.AddCookie(std::move(cookie));
    return ptr;
  }

  void RemoveSession(const std::string& sessionId) {
    std::lock_guard guard(mutex_);
    sessions_.erase(sessionId);
  }

  SessionPtr GetSession(const std::string& sessionId) const {
    std::lock_guard guard(mutex_);
    auto it = sessions_.find(sessionId);
    if (it != sessions_.end()) return it->second;
    return nullptr;
  }

 private:
  mutable std::mutex mutex_;
  std::map<std::string, SessionPtr> sessions_;
};

inline void HttpSession::Invalidate() { manager_->RemoveSession(sessionId_); }

}  // namespace Cold::Http

#endif /* COLD_HTTP_HTTPSESSION */
