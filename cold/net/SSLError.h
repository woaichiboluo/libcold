#ifndef COLD_NET_SSLERROR
#define COLD_NET_SSLERROR

#include <openssl/err.h>

namespace Cold {

struct SSLError {
  static unsigned long GetLastErrorCode() { return ERR_get_error(); }

  static const char* GetErrorStr() {
    static thread_local char t_err[512];
    return ERR_error_string(GetLastErrorCode(), t_err);
  }
};

}  // namespace Cold

#endif /* COLD_NET_SSLERROR */
