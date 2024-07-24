#ifndef NET_HTTP_HTTPSESSION
#define NET_HTTP_HTTPSESSION

#include <any>
#include <map>
#include <memory>
#include <string>
#include <type_traits>

#include "cold/thread/Lock.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#include "third_party/uuid/boost/uuid.hpp"
#pragma GCC diagnostic pop

namespace Cold::Net::Http {

class HttpSessionManager;

class HttpSession {
 public:
  HttpSession(std::string sessionId, HttpSessionManager* manager)
      : sessionId_(std::move(sessionId)), manager_(manager) {}
  ~HttpSession() = default;

  HttpSession(const HttpSession&) = delete;
  HttpSession& operator=(const HttpSession&) = delete;

  std::string_view GetSessionId() const { return sessionId_; }

  void Invalidate();
  // for attribute
  template <typename T>
  void SetAttribute(std::string key, T value) requires
      std::is_copy_assignable_v<T> && std::is_copy_constructible_v<T> {
    attributes_[std::move(key)] = std::any(std::move(value));
  }

  void SetAttribute(std::string key, const char* value) {
    attributes_[std::move(key)] = std::any(std::string(value));
  }

  template <typename T>
  T* GetAttribute(const std::string& key) {
    auto it = attributes_.find(key);
    if (it != attributes_.end()) {
      try {
        return std::any_cast<T>(&it->second);
      } catch (...) {
        return nullptr;
      }
    }
    return nullptr;
  }

  bool HasAttribute(const std::string& key) const {
    return attributes_.contains(key);
  }

  void RemoveAttribute(const std::string& key) { attributes_.erase(key); }

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

  SessionPtr CreateSession() {
    Base::LockGuard guard(mutex_);
    static boost::uuids::random_generator gen;
    auto uuid = boost::uuids::to_string(gen());
    auto session = std::make_shared<HttpSession>(uuid, this);
    sessions_[std::move(uuid)] = session;
    return session;
  }

  bool HasSession(const std::string& sessionId) const {
    Base::LockGuard guard(mutex_);
    return sessions_.contains(sessionId);
  }

  SessionPtr GetSession(const std::string& sessionId) const {
    Base::LockGuard guard(mutex_);
    auto it = sessions_.find(sessionId);
    if (it != sessions_.end()) return it->second;
    return nullptr;
  }

  void RemoveSession(const std::string& sessionId) {
    Base::LockGuard guard(mutex_);
    sessions_.erase(sessionId);
  }

 private:
  mutable Base::Mutex mutex_;
  std::map<std::string, SessionPtr> sessions_ GUARDED_BY(mutex_);
};

inline void HttpSession::Invalidate() { manager_->RemoveSession(sessionId_); }

}  // namespace Cold::Net::Http

#endif /* NET_HTTP_HTTPSESSION */
