#ifndef EXAMPLES_NET_SIMPLEHTTP_REQUESTPARSER
#define EXAMPLES_NET_SIMPLEHTTP_REQUESTPARSER

#include "HttpRequest.h"

class RequestParser {
 public:
  enum ParseState { kOK, kKEEP, kBAD };

  RequestParser() = default;
  ~RequestParser() = default;

  ParseState Parse(std::string_view content, HttpRequest& request);

  void Reset() {
    state_ = kMethod;
    unsolved_.clear();
  }

 private:
  enum State {
    kMethod,
    kUri,
    kRequestVersion,
    kHeaderName,
    kHeaderValue,
    kBody
  };

  static bool CheckMethod(std::string_view method, HttpRequest& request);
  static bool CheckUri(std::string_view uri, HttpRequest& request);
  static bool CheckVersion(std::string_view version, HttpRequest& request);
  static bool CheckHeaderName(std::string_view headerName,
                              HttpRequest& request);
  static bool CheckHeaderValue(std::string_view headerName,
                               std::string_view headerValue,
                               HttpRequest& request);
  static bool CheckBody(std::string_view body, HttpRequest& request);
  static size_t GetBodySize(HttpRequest& request);

  std::string unsolved_;
  std::string latestHeaderName_;
  State state_{kMethod};
};

#endif /* EXAMPLES_NET_SIMPLEHTTP_REQUESTPARSER */
