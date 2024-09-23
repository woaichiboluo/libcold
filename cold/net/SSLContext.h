#ifndef COLD_NET_SSLCONTEXT
#define COLD_NET_SSLCONTEXT

#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/types.h>

#include "../Log.h"
#include "SSLError.h"

namespace Cold {

class SSLContext {
 public:
  SSLContext(const SSLContext&) = delete;
  SSLContext& operator=(const SSLContext&) = delete;

  static void LoadCert(std::string_view cert, std::string_view key) {
    auto& sslContext = GetInstance();
    if (SSL_CTX_use_certificate_file(sslContext.context_, cert.data(),
                                     SSL_FILETYPE_PEM) <= 0) {
      FATAL("SSL_CTX_use_certificate_file failed. error: {}",
            SSLError::GetErrorStr());
    }
    if (SSL_CTX_use_PrivateKey_file(sslContext.context_, key.data(),
                                    SSL_FILETYPE_PEM) <= 0) {
      FATAL("SSL_CTX_use_PrivateKey_file failed. error: {}",
            SSLError::GetErrorStr());
    }
    if (!SSL_CTX_check_private_key(sslContext.context_)) {
      FATAL("SSL_CTX_check_private_key failed. error: {}",
            SSLError::GetErrorStr());
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
    if (SSL_library_init() != 1)
      FATAL("SSL_library_init failed. error: {}", SSLError::GetErrorStr());
    SSL_load_error_strings();
    ERR_load_crypto_strings();
    context_ = SSL_CTX_new(TLS_method());
    if (!context_)
      FATAL("SSL_CTX_new failed. error: {}", SSLError::GetErrorStr());
  }

  ~SSLContext() {
    if (context_) SSL_CTX_free(context_);
  }

  SSL_CTX* context_ = nullptr;
  bool certLoaded_ = false;
};

}  // namespace Cold
#endif /* COLD_NET_SSLCONTEXT */
