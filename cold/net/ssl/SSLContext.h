#ifndef NET_SSL_SSLCONTEXT
#define NET_SSL_SSLCONTEXT

#ifdef COLD_NET_ENABLE_SSL
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/types.h>

#include "cold/log/Logger.h"

namespace Cold::Net {

class SSLContext {
 public:
  ~SSLContext() {
    if (context_) SSL_CTX_free(context_);
  }

  SSLContext(const SSLContext&) = delete;
  SSLContext& operator=(const SSLContext&) = delete;

  static void LoadCert(std::string_view cert, std::string_view key) {
    auto& sslContext = GetInstance();
    if (SSL_CTX_use_certificate_file(sslContext.context_, cert.data(),
                                     SSL_FILETYPE_PEM) <= 0) {
      Base::FATAL("SSL_CTX_use_certificate_file failed");
    }
    if (SSL_CTX_use_PrivateKey_file(sslContext.context_, key.data(),
                                    SSL_FILETYPE_PEM) <= 0) {
      Base::FATAL("SSL_CTX_use_PrivateKey_file failed");
    }
    if (!SSL_CTX_check_private_key(sslContext.context_)) {
      Base::FATAL("SSL_CTX_check_private_key failed");
    }
    sslContext.certLoaded_ = true;
  }

  static SSLContext& GetInstance() {
    static SSLContext instance;
    return instance;
  }

  SSL_CTX* GetContext() { return context_; }

  bool CertLoaded() const { return certLoaded_; }

 private:
  SSLContext() {
    if (SSL_library_init() != 1) Base::FATAL("SSL_library_init failed");
    SSL_load_error_strings();
    context_ = SSL_CTX_new(TLS_method());
    if (!context_) Base::FATAL("SSL_CTX_new failed");
  }
  SSL_CTX* context_ = nullptr;
  bool certLoaded_ = false;
};

}  // namespace Cold::Net
#endif

#endif /* NET_SSL_SSLCONTEXT */
