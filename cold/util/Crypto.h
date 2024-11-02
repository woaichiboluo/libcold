#ifndef COLD_UTIL_CRYPTO
#define COLD_UTIL_CRYPTO

#ifdef COLD_ENABLE_SSL
#include <openssl/sha.h>

#include <string>

namespace Cold {

inline std::string SHA1(std::string_view input) {
  unsigned char hash[SHA_DIGEST_LENGTH];
  ::SHA1(reinterpret_cast<const unsigned char*>(input.data()), input.size(),
         hash);
  return {hash, hash + SHA_DIGEST_LENGTH};
}

}  // namespace Cold
#endif

#endif /* COLD_UTIL_CRYPTO */
