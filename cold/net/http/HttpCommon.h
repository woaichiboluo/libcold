#ifndef NET_HTTP_HTTPCOMMON
#define NET_HTTP_HTTPCOMMON

#include <cassert>
#include <cctype>
#include <cstdint>
#include <string>

#define HTTP_STATUS_MAP(XX)                                                   \
  XX(100, CONTINUE, CONTINUE)                                                 \
  XX(101, SWITCHING_PROTOCOLS, SWITCHING_PROTOCOLS)                           \
  XX(102, PROCESSING, PROCESSING)                                             \
  XX(103, EARLY_HINTS, EARLY_HINTS)                                           \
  XX(110, RESPONSE_IS_STALE, RESPONSE_IS_STALE)                               \
  XX(111, REVALIDATION_FAILED, REVALIDATION_FAILED)                           \
  XX(112, DISCONNECTED_OPERATION, DISCONNECTED_OPERATION)                     \
  XX(113, HEURISTIC_EXPIRATION, HEURISTIC_EXPIRATION)                         \
  XX(199, MISCELLANEOUS_WARNING, MISCELLANEOUS_WARNING)                       \
  XX(200, OK, OK)                                                             \
  XX(201, CREATED, CREATED)                                                   \
  XX(202, ACCEPTED, ACCEPTED)                                                 \
  XX(203, NON_AUTHORITATIVE_INFORMATION, NON_AUTHORITATIVE_INFORMATION)       \
  XX(204, NO_CONTENT, NO_CONTENT)                                             \
  XX(205, RESET_CONTENT, RESET_CONTENT)                                       \
  XX(206, PARTIAL_CONTENT, PARTIAL_CONTENT)                                   \
  XX(207, MULTI_STATUS, MULTI_STATUS)                                         \
  XX(208, ALREADY_REPORTED, ALREADY_REPORTED)                                 \
  XX(214, TRANSFORMATION_APPLIED, TRANSFORMATION_APPLIED)                     \
  XX(226, IM_USED, IM_USED)                                                   \
  XX(299, MISCELLANEOUS_PERSISTENT_WARNING, MISCELLANEOUS_PERSISTENT_WARNING) \
  XX(300, MULTIPLE_CHOICES, MULTIPLE_CHOICES)                                 \
  XX(301, MOVED_PERMANENTLY, MOVED_PERMANENTLY)                               \
  XX(302, FOUND, FOUND)                                                       \
  XX(303, SEE_OTHER, SEE_OTHER)                                               \
  XX(304, NOT_MODIFIED, NOT_MODIFIED)                                         \
  XX(305, USE_PROXY, USE_PROXY)                                               \
  XX(306, SWITCH_PROXY, SWITCH_PROXY)                                         \
  XX(307, TEMPORARY_REDIRECT, TEMPORARY_REDIRECT)                             \
  XX(308, PERMANENT_REDIRECT, PERMANENT_REDIRECT)                             \
  XX(400, BAD_REQUEST, BAD_REQUEST)                                           \
  XX(401, UNAUTHORIZED, UNAUTHORIZED)                                         \
  XX(402, PAYMENT_REQUIRED, PAYMENT_REQUIRED)                                 \
  XX(403, FORBIDDEN, FORBIDDEN)                                               \
  XX(404, NOT_FOUND, NOT_FOUND)                                               \
  XX(405, METHOD_NOT_ALLOWED, METHOD_NOT_ALLOWED)                             \
  XX(406, NOT_ACCEPTABLE, NOT_ACCEPTABLE)                                     \
  XX(407, PROXY_AUTHENTICATION_REQUIRED, PROXY_AUTHENTICATION_REQUIRED)       \
  XX(408, REQUEST_TIMEOUT, REQUEST_TIMEOUT)                                   \
  XX(409, CONFLICT, CONFLICT)                                                 \
  XX(410, GONE, GONE)                                                         \
  XX(411, LENGTH_REQUIRED, LENGTH_REQUIRED)                                   \
  XX(412, PRECONDITION_FAILED, PRECONDITION_FAILED)                           \
  XX(413, PAYLOAD_TOO_LARGE, PAYLOAD_TOO_LARGE)                               \
  XX(414, URI_TOO_LONG, URI_TOO_LONG)                                         \
  XX(415, UNSUPPORTED_MEDIA_TYPE, UNSUPPORTED_MEDIA_TYPE)                     \
  XX(416, RANGE_NOT_SATISFIABLE, RANGE_NOT_SATISFIABLE)                       \
  XX(417, EXPECTATION_FAILED, EXPECTATION_FAILED)                             \
  XX(418, IM_A_TEAPOT, IM_A_TEAPOT)                                           \
  XX(419, PAGE_EXPIRED, PAGE_EXPIRED)                                         \
  XX(420, ENHANCE_YOUR_CALM, ENHANCE_YOUR_CALM)                               \
  XX(421, MISDIRECTED_REQUEST, MISDIRECTED_REQUEST)                           \
  XX(422, UNPROCESSABLE_ENTITY, UNPROCESSABLE_ENTITY)                         \
  XX(423, LOCKED, LOCKED)                                                     \
  XX(424, FAILED_DEPENDENCY, FAILED_DEPENDENCY)                               \
  XX(425, TOO_EARLY, TOO_EARLY)                                               \
  XX(426, UPGRADE_REQUIRED, UPGRADE_REQUIRED)                                 \
  XX(428, PRECONDITION_REQUIRED, PRECONDITION_REQUIRED)                       \
  XX(429, TOO_MANY_REQUESTS, TOO_MANY_REQUESTS)                               \
  XX(430, REQUEST_HEADER_FIELDS_TOO_LARGE_UNOFFICIAL,                         \
     REQUEST_HEADER_FIELDS_TOO_LARGE_UNOFFICIAL)                              \
  XX(431, REQUEST_HEADER_FIELDS_TOO_LARGE, REQUEST_HEADER_FIELDS_TOO_LARGE)   \
  XX(440, LOGIN_TIMEOUT, LOGIN_TIMEOUT)                                       \
  XX(444, NO_RESPONSE, NO_RESPONSE)                                           \
  XX(449, RETRY_WITH, RETRY_WITH)                                             \
  XX(450, BLOCKED_BY_PARENTAL_CONTROL, BLOCKED_BY_PARENTAL_CONTROL)           \
  XX(451, UNAVAILABLE_FOR_LEGAL_REASONS, UNAVAILABLE_FOR_LEGAL_REASONS)       \
  XX(460, CLIENT_CLOSED_LOAD_BALANCED_REQUEST,                                \
     CLIENT_CLOSED_LOAD_BALANCED_REQUEST)                                     \
  XX(463, INVALID_X_FORWARDED_FOR, INVALID_X_FORWARDED_FOR)                   \
  XX(494, REQUEST_HEADER_TOO_LARGE, REQUEST_HEADER_TOO_LARGE)                 \
  XX(495, SSL_CERTIFICATE_ERROR, SSL_CERTIFICATE_ERROR)                       \
  XX(496, SSL_CERTIFICATE_REQUIRED, SSL_CERTIFICATE_REQUIRED)                 \
  XX(497, HTTP_REQUEST_SENT_TO_HTTPS_PORT, HTTP_REQUEST_SENT_TO_HTTPS_PORT)   \
  XX(498, INVALID_TOKEN, INVALID_TOKEN)                                       \
  XX(499, CLIENT_CLOSED_REQUEST, CLIENT_CLOSED_REQUEST)                       \
  XX(500, INTERNAL_SERVER_ERROR, INTERNAL_SERVER_ERROR)                       \
  XX(501, NOT_IMPLEMENTED, NOT_IMPLEMENTED)                                   \
  XX(502, BAD_GATEWAY, BAD_GATEWAY)                                           \
  XX(503, SERVICE_UNAVAILABLE, SERVICE_UNAVAILABLE)                           \
  XX(504, GATEWAY_TIMEOUT, GATEWAY_TIMEOUT)                                   \
  XX(505, HTTP_VERSION_NOT_SUPPORTED, HTTP_VERSION_NOT_SUPPORTED)             \
  XX(506, VARIANT_ALSO_NEGOTIATES, VARIANT_ALSO_NEGOTIATES)                   \
  XX(507, INSUFFICIENT_STORAGE, INSUFFICIENT_STORAGE)                         \
  XX(508, LOOP_DETECTED, LOOP_DETECTED)                                       \
  XX(509, BANDWIDTH_LIMIT_EXCEEDED, BANDWIDTH_LIMIT_EXCEEDED)                 \
  XX(510, NOT_EXTENDED, NOT_EXTENDED)                                         \
  XX(511, NETWORK_AUTHENTICATION_REQUIRED, NETWORK_AUTHENTICATION_REQUIRED)   \
  XX(520, WEB_SERVER_UNKNOWN_ERROR, WEB_SERVER_UNKNOWN_ERROR)                 \
  XX(521, WEB_SERVER_IS_DOWN, WEB_SERVER_IS_DOWN)                             \
  XX(522, CONNECTION_TIMEOUT, CONNECTION_TIMEOUT)                             \
  XX(523, ORIGIN_IS_UNREACHABLE, ORIGIN_IS_UNREACHABLE)                       \
  XX(524, TIMEOUT_OCCURED, TIMEOUT_OCCURED)                                   \
  XX(525, SSL_HANDSHAKE_FAILED, SSL_HANDSHAKE_FAILED)                         \
  XX(526, INVALID_SSL_CERTIFICATE, INVALID_SSL_CERTIFICATE)                   \
  XX(527, RAILGUN_ERROR, RAILGUN_ERROR)                                       \
  XX(529, SITE_IS_OVERLOADED, SITE_IS_OVERLOADED)                             \
  XX(530, SITE_IS_FROZEN, SITE_IS_FROZEN)                                     \
  XX(561, IDENTITY_PROVIDER_AUTHENTICATION_ERROR,                             \
     IDENTITY_PROVIDER_AUTHENTICATION_ERROR)                                  \
  XX(598, NETWORK_READ_TIMEOUT, NETWORK_READ_TIMEOUT)                         \
  XX(599, NETWORK_CONNECT_TIMEOUT, NETWORK_CONNECT_TIMEOUT)

namespace Cold::Net::Http {

constexpr const std::string_view kCRLF = "\r\n";

enum HttpStatus {
#define XX(value, name, name1) name = value,
  HTTP_STATUS_MAP(XX)
#undef XX
};

inline const char* HttpStatusToHttpStatusMsg(HttpStatus status) {
  switch (status) {
#define XX(value, name, name1) \
  case HttpStatus::name:       \
    return #name;
    HTTP_STATUS_MAP(XX)
#undef XX
  }
  assert(0);
}

inline std::string UrlEncode(std::string_view url) {
  static constexpr const std::string_view k_HEX = "0123456789ABCDEF";
  std::string ret;
  ret.reserve(url.size());
  for (const auto& c : url) {
    if (isalpha(c) || isdigit(c))
      ret.push_back(c);
    else {
      ret.push_back('%');
      ret.push_back(k_HEX[static_cast<uint8_t>(c) >> 4]);
      ret.push_back(k_HEX[c & 0xF]);
    }
  }
  return ret;
}

inline std::string UrlDecode(std::string_view url) {
  constexpr static auto HexToNum = [](char c) -> uint8_t {
    auto h = toupper(c);
    return static_cast<uint8_t>('A' <= h && h <= 'Z' ? h - 'A' + 10 : h - '0');
  };
  std::string ret;
  for (size_t i = 0; i < url.size(); ++i) {
    if (url[i] == '+') {
      ret.push_back(' ');
    } else if (url[i] == '%' && i + 2 < url.size() && isxdigit(url[i + 1]) &&
               isxdigit(url[i + 2])) {
      ret.push_back(static_cast<char>((HexToNum(url[i + 1]) << 4) |
                                      HexToNum(url[i + 2])));
      i += 2;
    } else {
      ret.push_back(url[i]);
    }
  }
  return ret;
}

}  // namespace Cold::Net::Http

#endif /* NET_HTTP_HTTPCOMMON */
