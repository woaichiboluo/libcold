#ifndef COLD_HTTP_STATICFILESEVLET
#define COLD_HTTP_STATICFILESEVLET

#include <fcntl.h>
#include <sys/mman.h>

#include <cassert>
#include <filesystem>
#include <string>

#include "HttpRequest.h"
#include "HttpServlet.h"
#include "MimeTypes.h"

namespace Cold::Http {

class StaticFileBody : public HttpBody {
 public:
  StaticFileBody(std::string requestPath)
      : requestPath_(std::move(requestPath)), guard_([this]() {
          if (fd_ >= 0) close(fd_);
        }) {
    // get suffix
    auto pos = requestPath_.find_last_of('.');
    if (pos != std::string::npos) {
      suffix_ = std::string_view(requestPath_).substr(pos + 1);
    }
    // open file
    fd_ = open(requestPath_.data(), O_RDONLY | O_CLOEXEC);
    if (fd_ < 0) {
      return;
    } else {
      std::filesystem::path file(requestPath_);
      std::error_code ec;
      fileSize_ = std::filesystem::file_size(file, ec);
      assert(ec == std::error_code());
    }
  }

  ~StaticFileBody() = default;

  void SetRelatedHeaders(std::map<std::string, std::string>& headers) override {
    headers["Content-Type"] = GetMimeType(suffix_);
    headers["Content-Length"] = fmt::format("{}", fileSize_);
  }

  Task<bool> SendBody(TcpSocket& socket) override {
    assert(fd_ >= 0);
    auto addr = mmap(nullptr, fileSize_, PROT_READ, MAP_PRIVATE, fd_, 0);
    if (addr == MAP_FAILED) {
      co_return false;
    }
    ScopeGuard guard([addr, size = fileSize_] { munmap(addr, size); });
    auto n =
        co_await socket.WriteN(reinterpret_cast<const char*>(addr), fileSize_);
    co_return n == static_cast<ssize_t>(fileSize_);
  }

  std::string ToRawBody() const override {
    std::string res;
    char buf[65536];
    while (res.size() != fileSize_) {
      auto n = read(fd_, buf, sizeof buf);
      if (n <= 0) break;
      res.append(buf, static_cast<size_t>(n));
    }
    return res;
  }

  bool StaticFileAvailable() const { return fd_ >= 0; }

 private:
  std::string requestPath_;
  std::string_view suffix_;
  size_t fileSize_ = 0;
  int fd_ = -1;
  ScopeGuard<std::function<void()> > guard_;
};

class StaticFileServlet : public HttpServlet {
 public:
  StaticFileServlet(std::string path) : rootPath_(std::move(path)) {
    assert(rootPath_.find("..") == std::string::npos);
  }
  ~StaticFileServlet() override = default;

  void DoService(HttpRequest& request, HttpResponse& response) override {
    auto requestPath =
        fmt::format("{}{}", rootPath_,
                    request.GetUrl() == "/" ? "/index.html" : request.GetUrl());
    if (requestPath.find("..") != std::string::npos) {
      response.SetHttpStatusCode(k403);
    } else {
      std::filesystem::path file(requestPath);
      if (!std::filesystem::exists(file)) {
        response.SetHttpStatusCode(k404);
      } else {
        auto status = std::filesystem::status(file);
        auto readable =
            (status.permissions() & std::filesystem::perms::others_read) !=
            std::filesystem::perms::none;
        if (std::filesystem::is_regular_file(status) && readable) {
          auto body = MakeHttpBody<StaticFileBody>(requestPath);
          if (!body->StaticFileAvailable()) {
            response.SetHttpStatusCode(k500);
          } else {
            response.SetBody(std::move(body));
          }
        } else {
          response.SetHttpStatusCode(k403);
        }
      }
    }
    if (response.GetHttpStatusCode() != k200) {
      auto body = std::make_unique<HtmlTextBody>();
      body->Append(GetDefaultErrorPage(response.GetHttpStatusCode()));
      response.SetBody(std::move(body));
    }
  }

 private:
  std::string rootPath_;
};

}  // namespace Cold::Http

#endif /* COLD_HTTP_STATICFILESEVLET */
