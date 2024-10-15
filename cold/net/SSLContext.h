#ifndef COLD_NET_SSLCONTEXT
#define COLD_NET_SSLCONTEXT

#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/types.h>

#include "../log/Log.h"

namespace Cold {

struct SSLError {
  static unsigned long GetLastErrorCode() { return ERR_get_error(); }

  static int GetSSLErrorCode(SSL* ssl, int ret) {
    return SSL_get_error(ssl, ret);
  }

  static const char* GetErrorStr(unsigned long err) {
    static thread_local char t_err[512];
    return ERR_error_string(err, t_err);
  }

  static const char* GetLastErrorStr() {
    return GetErrorStr(GetLastErrorCode());
  }
};

class SSLContext {
 public:
  SSLContext() {
    static std::once_flag flag;
    std::call_once(flag, []() {
      SSL_load_error_strings();
      ERR_load_crypto_strings();
      if (SSL_library_init() != 1) {
        FATAL("SSL_library_init failed. error: {}",
              SSLError::GetLastErrorStr());
      }
    });
    context_ = SSL_CTX_new(TLS_method());
    if (!context_)
      ERROR("SSL_CTX_new failed. error: {}", SSLError::GetLastErrorStr());
  }

  ~SSLContext() {
    if (context_) SSL_CTX_free(context_);
  }

  SSLContext(const SSLContext&) = delete;
  SSLContext& operator=(const SSLContext&) = delete;

  SSLContext(SSLContext&& other) noexcept : context_(other.context_) {
    other.context_ = nullptr;
  }

  SSLContext& operator=(SSLContext&& other) noexcept {
    if (this == &other) {
      return *this;
    }
    if (context_) SSL_CTX_free(context_);
    context_ = other.context_;
    other.context_ = nullptr;
    return *this;
  }

  SSL_CTX* GetNativeHandle() const { return context_; }

  void LoadCert(std::string_view cert, std::string_view key) {
    if (SSL_CTX_use_certificate_file(context_, cert.data(), SSL_FILETYPE_PEM) <=
        0) {
      FATAL("SSL_CTX_use_certificate_file failed.  {}",
            SSLError::GetLastErrorStr());
    }
    if (SSL_CTX_use_PrivateKey_file(context_, key.data(), SSL_FILETYPE_PEM) <=
        0) {
      FATAL("SSL_CTX_use_PrivateKey_file failed.  {}",
            SSLError::GetLastErrorStr());
    }
    if (!SSL_CTX_check_private_key(context_)) {
      FATAL("SSL_CTX_check_private_key failed.  {}",
            SSLError::GetLastErrorStr());
    }
    certLoaded_ = true;
  }

  bool IsCertLoaded() const { return certLoaded_; }

  void SetCertLoaded(bool loaded) { certLoaded_ = loaded; }

  static SSLContext& GetGlobalDefaultInstance() {
    static SSLContext instance;
    assert(instance.context_);
    return instance;
  }

 private:
  SSL_CTX* context_ = nullptr;
  bool certLoaded_ = false;
};

class SSLHolder {
 public:
  SSLHolder() = default;

  SSLHolder(SSL* ssl) : ssl_(ssl) {}

  ~SSLHolder() {
    if (ssl_) SSL_free(ssl_);
  }

  SSLHolder(const SSLHolder&) = delete;
  SSLHolder& operator=(const SSLHolder&) = delete;

  SSLHolder(SSLHolder&& other) noexcept : ssl_(other.ssl_) {
    other.ssl_ = nullptr;
  }

  SSLHolder& operator=(SSLHolder&& other) noexcept {
    if (this == &other) {
      return *this;
    }
    if (ssl_) SSL_free(ssl_);
    ssl_ = other.ssl_;
    other.ssl_ = nullptr;
    return *this;
  }

  operator bool() const { return ssl_ != nullptr; }

  SSL* GetNativeHandle() const { return ssl_; }

  int GetSSLErrorCode(int ret) const {
    return SSLError::GetSSLErrorCode(ssl_, ret);
  }

 private:
  SSL* ssl_ = nullptr;
};

}  // namespace Cold
#endif /* COLD_NET_SSLCONTEXT */
