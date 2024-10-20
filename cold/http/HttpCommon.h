#ifndef COLD_HTTP_HTTPCOMMON
#define COLD_HTTP_HTTPCOMMON

#include <uuid/uuid.h>

#include <string>
#include <string_view>

#include "../detail/fmt.h"

#define STRING_MAP_CRUD(MethodName, Member)                              \
  void Set##MethodName(std::string key, auto&& value) {                  \
    Member[std::move(key)] = std::move(value);                           \
  }                                                                      \
  bool Has##MethodName(const std::string& key) const {                   \
    return Member.contains(key);                                         \
  }                                                                      \
  std::string_view Get##MethodName(const std::string& key) const {       \
    auto it = Member.find(key);                                          \
    if (it == Member.end()) return {};                                   \
    return it->second;                                                   \
  }                                                                      \
  void Remove##MethodName(const std::string& key) { Member.erase(key); } \
  auto& GetAll##MethodName() { return Member; }                          \
  const auto& GetAll##MethodName() const { return Member; }

#define ANY_MAP_CRUD(MethodName, Member)                                 \
  template <typename T>                                                  \
  void Set##MethodName(std::string key, T value) {                       \
    Member[std::move(key)] = std::any(std::move(value));                 \
  }                                                                      \
  void Set##MethodName(std::string key, const char* value) {             \
    Member[std::move(key)] = std::any(std::string(value));               \
  }                                                                      \
  bool Has##MethodName(const std::string& key) const {                   \
    return Member.contains(key);                                         \
  }                                                                      \
  template <typename T>                                                  \
  T* Get##MethodName(const std::string& key) {                           \
    auto it = Member.find(key);                                          \
    if (it == Member.end()) return nullptr;                              \
    try {                                                                \
      return std::any_cast<T>(&it->second);                              \
    } catch (...) {                                                      \
      return nullptr;                                                    \
    }                                                                    \
  }                                                                      \
  void Remove##MethodName(const std::string& key) { Member.erase(key); } \
  auto& GetAll##MethodName() { return Member; }                          \
  const auto& GetAll##MethodName() const { return Member; }

#define HTTP_STATUS_MESSAGE_MAP(XX)                                           \
  XX(100, CONTINUE, Continue)                                                 \
  XX(101, SWITCHING_PROTOCOLS, Switching Protocols)                           \
  XX(102, PROCESSING, Processing)                                             \
  XX(103, EARLY_HINTS, Early Hints)                                           \
  XX(110, RESPONSE_IS_STALE, Response Is Stale)                               \
  XX(111, REVALIDATION_FAILED, Revalidation Failed)                           \
  XX(112, DISCONNECTED_OPERATION, Disconnected Operation)                     \
  XX(113, HEURISTIC_EXPIRATION, Heuristic ExpiratioN)                         \
  XX(199, MISCELLANEOUS_WARNING, Miscellaneous Warning)                       \
  XX(200, OK, OK)                                                             \
  XX(201, CREATED, Created)                                                   \
  XX(202, ACCEPTED, Accepted)                                                 \
  XX(203, NON_AUTHORITATIVE_INFORMATION, Non Authoritative Information)       \
  XX(204, NO_CONTENT, No Content)                                             \
  XX(205, RESET_CONTENT, Reset Content)                                       \
  XX(206, PARTIAL_CONTENT, Partial Content)                                   \
  XX(207, MULTI_STATUS, Multi - Status)                                       \
  XX(208, ALREADY_REPORTED, Already Reported)                                 \
  XX(214, TRANSFORMATION_APPLIED, Transformation Applied)                     \
  XX(226, IM_USED, IM Used)                                                   \
  XX(299, MISCELLANEOUS_PERSISTENT_WARNING, Miscellaneous Persistent Warning) \
  XX(300, MULTIPLE_CHOICES, Multiple Choices)                                 \
  XX(301, MOVED_PERMANENTLY, Moved Permanently)                               \
  XX(302, FOUND, Found)                                                       \
  XX(303, SEE_OTHER, See Other)                                               \
  XX(304, NOT_MODIFIED, Not Modified)                                         \
  XX(305, USE_PROXY, Use Proxy)                                               \
  XX(306, SWITCH_PROXY, Switch Proxy)                                         \
  XX(307, TEMPORARY_REDIRECT, Temporary Redirect)                             \
  XX(308, PERMANENT_REDIRECT, Permanent Redirect)                             \
  XX(400, BAD_REQUEST, Bad Request)                                           \
  XX(401, UNAUTHORIZED, Unauthorized)                                         \
  XX(402, PAYMENT_REQUIRED, Payment Required)                                 \
  XX(403, FORBIDDEN, Forbidden)                                               \
  XX(404, NOT_FOUND, Not Found)                                               \
  XX(405, METHOD_NOT_ALLOWED, Method Not Allowed)                             \
  XX(406, NOT_ACCEPTABLE, Not Acceptable)                                     \
  XX(407, PROXY_AUTHENTICATION_REQUIRED, Proxy Authentication Required)       \
  XX(408, REQUEST_TIMEOUT, Request Timeout)                                   \
  XX(409, CONFLICT, Conflict)                                                 \
  XX(410, GONE, Gone)                                                         \
  XX(411, LENGTH_REQUIRED, Length Required)                                   \
  XX(412, PRECONDITION_FAILED, Precondition Failed)                           \
  XX(413, PAYLOAD_TOO_LARGE, Payload Too Large)                               \
  XX(414, URI_TOO_LONG, URI Too Long)                                         \
  XX(415, UNSUPPORTED_MEDIA_TYPE, Unsupported Media Type)                     \
  XX(416, RANGE_NOT_SATISFIABLE, Range Not Satisfiable)                       \
  XX(417, EXPECTATION_FAILED, Expectation Failed)                             \
  XX(418, IM_A_TEAPOT, Im a teapot)                                           \
  XX(419, PAGE_EXPIRED, Page Expired)                                         \
  XX(420, ENHANCE_YOUR_CALM, Enhance Your Calm)                               \
  XX(421, MISDIRECTED_REQUEST, Misdirected Request)                           \
  XX(422, UNPROCESSABLE_ENTITY, Unprocessable Entity)                         \
  XX(423, LOCKED, Locked)                                                     \
  XX(424, FAILED_DEPENDENCY, Failed Dependency)                               \
  XX(425, TOO_EARLY, Too Early)                                               \
  XX(426, UPGRADE_REQUIRED, Upgrade Required)                                 \
  XX(428, PRECONDITION_REQUIRED, Precondition Required)                       \
  XX(429, TOO_MANY_REQUESTS, Too Many Requests)                               \
  XX(430, REQUEST_HEADER_FIELDS_TOO_LARGE_UNOFFICIAL,                         \
     Request Header Fields Too Large Unofficial)                              \
  XX(431, REQUEST_HEADER_FIELDS_TOO_LARGE, Request Header Fields Too Large)   \
  XX(440, LOGIN_TIMEOUT, Login Timeout)                                       \
  XX(444, NO_RESPONSE, No Response)                                           \
  XX(449, RETRY_WITH, Retry With)                                             \
  XX(450, BLOCKED_BY_PARENTAL_CONTROL, Blocked by Parental Control)           \
  XX(451, UNAVAILABLE_FOR_LEGAL_REASONS, Unavailable For Legal Reasons)       \
  XX(460, CLIENT_CLOSED_LOAD_BALANCED_REQUEST,                                \
     Client Closed Load Balanced Request)                                     \
  XX(463, INVALID_X_FORWARDED_FOR, Invalid X Forwarded For)                   \
  XX(494, REQUEST_HEADER_TOO_LARGE, Request Header Too Large)                 \
  XX(495, SSL_CERTIFICATE_ERROR, SSL Certificate Error)                       \
  XX(496, SSL_CERTIFICATE_REQUIRED, SSL Certificate Required)                 \
  XX(497, HTTP_REQUEST_SENT_TO_HTTPS_PORT, HTTP Request Sent to HTTPS Port)   \
  XX(498, INVALID_TOKEN, Invalid Token)                                       \
  XX(499, CLIENT_CLOSED_REQUEST, Client Closed Request)                       \
  XX(500, INTERNAL_SERVER_ERROR, Internal Server Error)                       \
  XX(501, NOT_IMPLEMENTED, Not Implemented)                                   \
  XX(502, BAD_GATEWAY, Bad Gateway)                                           \
  XX(503, SERVICE_UNAVAILABLE, Service Unavailable)                           \
  XX(504, GATEWAY_TIMEOUT, Gateway Timeout)                                   \
  XX(505, HTTP_VERSION_NOT_SUPPORTED, HTTP Version Not Supported)             \
  XX(506, VARIANT_ALSO_NEGOTIATES, Variant Also Negotiates)                   \
  XX(507, INSUFFICIENT_STORAGE, Insufficient Storage)                         \
  XX(508, LOOP_DETECTED, Loop Detected)                                       \
  XX(509, BANDWIDTH_LIMIT_EXCEEDED, Bandwidth Limit Exceeded)                 \
  XX(510, NOT_EXTENDED, Not Extended)                                         \
  XX(511, NETWORK_AUTHENTICATION_REQUIRED, Network Authentication Required)   \
  XX(520, WEB_SERVER_UNKNOWN_ERROR, Web Server Unknown Error)                 \
  XX(521, WEB_SERVER_IS_DOWN, Web Server Is Down)                             \
  XX(522, CONNECTION_TIMEOUT, Connection Timeout)                             \
  XX(523, ORIGIN_IS_UNREACHABLE, Origin Is Unreachable)                       \
  XX(524, TIMEOUT_OCCURED, Timeout Occurred)                                  \
  XX(525, SSL_HANDSHAKE_FAILED, SSL Handshake Failed)                         \
  XX(526, INVALID_SSL_CERTIFICATE, Invalid SSL Certificate)                   \
  XX(527, RAILGUN_ERROR, Railgun Error)                                       \
  XX(529, SITE_IS_OVERLOADED, Site Is Overloaded)                             \
  XX(530, SITE_IS_FROZEN, Site Is Frozen)                                     \
  XX(561, IDENTITY_PROVIDER_AUTHENTICATION_ERROR,                             \
     Identity Provider Authentication Error)                                  \
  XX(598, NETWORK_READ_TIMEOUT, Network Read Timeout)                         \
  XX(599, NETWORK_CONNECT_TIMEOUT, Network Connect Timeout)

namespace Cold::Http {
static constexpr std::string_view kCRLF = "\r\n";

enum HttpStatusCode {
#define XX(value, name, message) k##value = value,
  HTTP_STATUS_MESSAGE_MAP(XX)
#undef XX
};

inline HttpStatusCode IntToHttpStatusCode(unsigned int code) {
  return static_cast<HttpStatusCode>(code);
}

inline unsigned int HttpStatusCodeToInt(HttpStatusCode code) {
  return static_cast<unsigned int>(code);
}

inline std::string_view GetHttpStatusCodeMessage(HttpStatusCode code) {
  switch (code) {
#define XX(value, name, message) \
  case k##value:                 \
    return #message;
    HTTP_STATUS_MESSAGE_MAP(XX)
#undef XX
    default:
      return "";
  }
}

enum HttpVersion { kHTTP_V_10, kHTTP_V_11 };

inline std::string_view HttpVersionToStr(HttpVersion version) {
  switch (version) {
    case kHTTP_V_10:
      return "HTTP/1.0";
    case kHTTP_V_11:
      return "HTTP/1.1";
    default:
      return "HTTP/1.1";
  }
}

inline HttpVersion StrToHttpVersion(std::string_view version) {
  if (version == "HTTP/1.0") {
    return kHTTP_V_10;
  } else {
    return kHTTP_V_11;
  }
}

enum HttpMethod {
  kGET,
  kPOST,
  kPUT,
  kDELETE,
  kHEAD,
  kOPTIONS,
  kTRACE,
  kCONNECT,
  kPATCH,
};

inline std::string_view HttpMethodToStr(HttpMethod method) {
  switch (method) {
    case kGET:
      return "GET";
    case kPOST:
      return "POST";
    case kPUT:
      return "PUT";
    case kDELETE:
      return "DELETE";
    case kHEAD:
      return "HEAD";
    case kOPTIONS:
      return "OPTIONS";
    case kTRACE:
      return "TRACE";
    case kCONNECT:
      return "CONNECT";
    case kPATCH:
      return "PATCH";
  }
}

inline HttpMethod StrToHttpMethod(std::string_view method) {
  if (method == "GET") {
    return kGET;
  } else if (method == "POST") {
    return kPOST;
  } else if (method == "PUT") {
    return kPUT;
  } else if (method == "DELETE") {
    return kDELETE;
  } else if (method == "HEAD") {
    return kHEAD;
  } else if (method == "OPTIONS") {
    return kOPTIONS;
  } else if (method == "TRACE") {
    return kTRACE;
  } else if (method == "CONNECT") {
    return kCONNECT;
  } else {
    return kPATCH;
  }
}

enum HttpConnectionStatus { kKeepAlive, kClose, kUpgrade };

inline std::string_view HttpConnectionStatusToStr(HttpConnectionStatus status) {
  switch (status) {
    case kKeepAlive:
      return "keep-alive";
    case kClose:
      return "close";
    case kUpgrade:
      return "Upgrade";
    default:
      return "close";
  }
}

inline std::string GenerateUUID() {
  uuid_t uuid;
  char buf[256]{};
  uuid_generate_random(uuid);
  uuid_unparse_lower(uuid, buf);
  return buf;
}

inline std::string GetDefaultErrorPage(HttpStatusCode code) {
  auto status = HttpStatusCodeToInt(code);
  auto message = GetHttpStatusCodeMessage(code);
  return fmt::format(
      "<html><head><title>{} {}</title></head><body><center><h1>{} "
      "{}</h1></center><hr><center>Cold-HTTP</center></body></html>",
      status, message, status, message);
}

}  // namespace Cold::Http

#endif /* COLD_HTTP_HTTPCOMMON */
