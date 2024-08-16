#ifndef COLD_UTIL_CRYPTO
#define COLD_UTIL_CRYPTO

#ifdef COLD_NET_ENABLE_SSL
#include <openssl/sha.h>
#endif

#include <string>
#include <string_view>

namespace Cold::Base {

#ifdef COLD_NET_ENABLE_SSL
inline std::string SHA1(std::string_view input) {
  unsigned char hash[SHA_DIGEST_LENGTH];
  ::SHA1(reinterpret_cast<const unsigned char*>(input.data()), input.size(),
         hash);
  return {hash, hash + SHA_DIGEST_LENGTH};
}
#endif

inline std::string Base64Decode(std::string_view input) {
  std::string result;
  result.resize(input.size() * 3 / 4);
  char* writeBuf = &result[0];

  const char* ptr = input.data();
  const char* end = ptr + input.size();

  while (ptr < end) {
    size_t i = 0;
    size_t padding = 0;
    size_t packed = 0;
    for (; i < 4 && ptr < end; ++i, ++ptr) {
      if (*ptr == '=') {
        ++padding;
        packed <<= 6;
        continue;
      }

      // padding with "=" only
      if (padding > 0) {
        return "";
      }

      int val = 0;
      if (*ptr >= 'A' && *ptr <= 'Z') {
        val = *ptr - 'A';
      } else if (*ptr >= 'a' && *ptr <= 'z') {
        val = *ptr - 'a' + 26;
      } else if (*ptr >= '0' && *ptr <= '9') {
        val = *ptr - '0' + 52;
      } else if (*ptr == '+') {
        val = 62;
      } else if (*ptr == '/') {
        val = 63;
      } else {
        return "";  // invalid character
      }

      packed = (packed << 6) | static_cast<size_t>(val);
    }
    if (i != 4) {
      return "";
    }
    if (padding > 0 && ptr != end) {
      return "";
    }
    if (padding > 2) {
      return "";
    }

    *writeBuf++ = (char)((packed >> 16) & 0xff);
    if (padding != 2) {
      *writeBuf++ = (char)((packed >> 8) & 0xff);
    }
    if (padding == 0) {
      *writeBuf++ = (char)(packed & 0xff);
    }
  }

  result.resize(static_cast<size_t>(writeBuf - result.data()));
  return result;
}

inline std::string Base64Encode(std::string_view input) {
  constexpr const char* kBase64 =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  std::string ret;

  const unsigned char* ptr =
      reinterpret_cast<const unsigned char*>(input.data());
  const unsigned char* end =
      reinterpret_cast<const unsigned char*>(input.data() + input.size());

  while (ptr < end) {
    unsigned int packed = 0;
    size_t i = 0;
    size_t padding = 0;
    for (; i < 3 && ptr < end; ++i, ++ptr) {
      packed = (packed << 8) | *ptr;
    }
    if (i == 2) {
      padding = 1;
    } else if (i == 1) {
      padding = 2;
    }
    for (; i < 3; ++i) {
      packed <<= 8;
    }

    ret.append(1, kBase64[packed >> 18]);
    ret.append(1, kBase64[(packed >> 12) & 0x3f]);
    if (padding != 2) {
      ret.append(1, kBase64[(packed >> 6) & 0x3f]);
    }
    if (padding == 0) {
      ret.append(1, kBase64[packed & 0x3f]);
    }
    ret.append(padding, '=');
  }

  return ret;
}

}  // namespace Cold::Base

#endif /* COLD_UTIL_CRYPTO */
