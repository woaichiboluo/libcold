#include "examples/simple/simplehttp/HttpResponse.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cassert>
#include <string>

#include "cold/net/TcpSocket.h"
#include "cold/util/ScopeUtil.h"

namespace {
const std::map<std::string, std::string> kMimeType = {
    {"gif", "image/gif"},      {"htm", "text/html"}, {"html", "text/html"},
    {"jpg", "image/jpeg"},     {"png", "image/png"}, {"mp4", "video/mp4"},
    {"js", "text/javascript"}, {"css", "text/css"}};

const std::map<int, std::string_view> kFirstLineWithoutVersion = {
    {200, " 200 OK\r\n"},
    {400, " 400 Bad Request\r\n"},
    {403, " 403 Forbidden\r\n"},
    {404, " 404 Not Found\r\n"},
    {500, " 500 Internal Server Error\r\n"},
};

const std::map<int, std::string_view> kBodys{
    {400,
     "<html>"
     "<head><title>Bad Request</title></head>"
     "<body><h1>400 Bad Request</h1></body>"
     "</html>"},
    {403,
     "<html>"
     "<head><title>Forbidden</title></head>"
     "<body><h1>403 Forbidden</h1></body>"
     "</html>"},
    {404,
     "<html>"
     "<head><title>Not Found</title></head>"
     "<body><h1>404 Not Found</h1></body>"
     "</html>"},
    {500,
     "<html>"
     "<head><title>Internal Server Error</title></head>"
     "<body><h1>500 Internal Server Error</h1></body>"
     "</html>"},
};
}  // namespace

HttpResponse::~HttpResponse() {
  if (mmapAddr_) {
    munmap(mmapAddr_->first, mmapAddr_->second);
  }
}

void HttpResponse::MakeResponse(bool ok, HttpRequest& request,
                                HttpResponse& response) {
  std::string_view uri = request.GetUri();
  std::string_view version = request.GetVersion();
  if (version == "") version = "HTTP/1.1";
  if (!ok || uri.empty() || uri[0] != '/') {
    response.MakeResponseHeaders(400, version);
    return;
  }
  if (uri == "/") uri = "/index.html";
  uri = uri.substr(1);
  auto pos = uri.find_last_of(".");
  std::string ext;
  if (pos != std::string_view::npos) {
    auto it = kMimeType.find(std::string(uri.substr(pos + 1)));
    ext = it == kMimeType.end() ? "text/html" : it->second;
  }
  // try open file
  std::string path{uri};
  struct stat s;
  if (stat(path.data(), &s) < 0) {
    response.MakeResponseHeaders(errno == ENOENT ? 404 : 500, version);
    return;
  }
  if (S_ISDIR(s.st_mode) || (S_IROTH & s.st_mode) == 0) {
    response.MakeResponseHeaders(404, version);
    return;
  }
  int fd = open(path.data(), O_RDONLY);
  Cold::Base::ScopeGuard guard([fd]() { close(fd); });
  if (fd < 0) {
    response.MakeResponseHeaders(500, version);
    return;
  }
  auto addr = mmap(nullptr, static_cast<size_t>(s.st_size), PROT_READ,
                   MAP_PRIVATE, fd, 0);
  if (addr == MAP_FAILED) {
    response.MakeResponseHeaders(500, version);
    return;
  }
  response.mmapAddr_ = {addr, s.st_size};
  response.headers_["Content-Type"] = ext;
  response.MakeResponseHeaders(200, version);
  return;
}

void HttpResponse::MakeResponseHeaders(int status, std::string_view version) {
  buf_.insert(buf_.end(), version.begin(), version.end());
  assert(kFirstLineWithoutVersion.count(status));
  auto firstline = kFirstLineWithoutVersion.find(status)->second;
  buf_.insert(buf_.end(), firstline.begin(), firstline.end());
  if (mmapAddr_) {
    headers_["Content-Length"] = std::to_string(mmapAddr_->second);
  } else {
    auto body = kBodys.find(status)->second;
    headers_["Content-Length"] = std::to_string(body.length());
  }
  constexpr std::string_view crlf = "\r\n";
  for (const auto& [key, value] : headers_) {
    buf_.insert(buf_.end(), key.begin(), key.end());
    buf_.push_back(':');
    buf_.push_back(' ');
    buf_.insert(buf_.end(), value.begin(), value.end());
    buf_.insert(buf_.end(), crlf.begin(), crlf.end());
  }
  buf_.insert(buf_.end(), crlf.begin(), crlf.end());
  if (status != 200) {
    auto body = kBodys.find(status)->second;
    buf_.insert(buf_.end(), body.begin(), body.end());
  }
}

Cold::Base::Task<ssize_t> HttpResponse::SendRequest(
    Cold::Net::TcpSocket& socket) {
  auto ret = co_await socket.Write(buf_.data(), buf_.size());
  if (ret < 0) co_return ret;
  if (mmapAddr_) {
    ret = co_await socket.WriteN(static_cast<const char*>(mmapAddr_->first),
                                 mmapAddr_->second);
  }
  co_return ret;
}