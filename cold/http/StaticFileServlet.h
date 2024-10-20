#ifndef COLD_HTTP_STATICFILESEVLET
#define COLD_HTTP_STATICFILESEVLET

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <cassert>
#include <string>

#include "HttpRequest.h"
#include "HttpServlet.h"
#include "MimeTypes.h"

namespace Cold::Http {

class StaticFileBody : public HttpBody {
 public:
  StaticFileBody(std::string requestPath, struct stat st)
      : requestPath_(std::move(requestPath)), st_(st), guard_([this]() {
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
    }
  }

  ~StaticFileBody() = default;

  void SetRelatedHeaders(std::map<std::string, std::string>& headers) override {
    headers["Content-Type"] = GetMimeType(suffix_);
    headers["Content-Length"] = fmt::format("{}", st_.st_size);
    if (requestPath_ == "/index.html") {
      headers["Cache-Control"] = "no-cache";
    } else {
      headers["Cache-Control"] = "public, max-age=3600";
    }
    headers["ETag"] = fmt::format("{:x}-{:x}", st_.st_ino, st_.st_mtime);
  }

  Task<bool> SendBody(TcpSocket& socket) override {
    assert(fd_ >= 0);
    if (st_.st_size < 0) {
      co_return false;
    }
    auto fileSize = static_cast<size_t>(st_.st_size);
    auto addr = mmap(nullptr, fileSize, PROT_READ, MAP_PRIVATE, fd_, 0);
    if (addr == MAP_FAILED) {
      co_return false;
    }
    ScopeGuard guard([addr, size = fileSize] { munmap(addr, size); });
    auto n =
        co_await socket.WriteN(reinterpret_cast<const char*>(addr), fileSize);
    co_return n == static_cast<ssize_t>(fileSize);
  }

  std::string ToRawBody() const override {
    std::string res;
    char buf[65536];
    if (fd_ < 0) return res;
    while (res.size() != static_cast<size_t>(st_.st_size)) {
      auto n = read(fd_, buf, sizeof buf);
      if (n <= 0) break;
      res.append(buf, static_cast<size_t>(n));
    }
    return res;
  }

  bool StaticFileAvailable() const { return fd_ >= 0; }

 private:
  std::string requestPath_;
  struct stat st_;
  std::string_view suffix_;
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
    do {
      if (requestPath.find("..") != std::string::npos) {
        response.SetHttpStatusCode(k403);
        break;
      }
      struct stat st;
      if (stat(requestPath.data(), &st) != 0) {
        response.SetHttpStatusCode(errno == ENOENT ? k404 : k500);
        break;
      }
      bool isRegularFile = (st.st_mode & S_IFREG) != 0;
      bool reable = (st.st_mode & S_IROTH) != 0;
      if (!isRegularFile || !reable) {
        response.SetHttpStatusCode(k403);
        break;
      }
      // check cache
      auto etag = request.GetHeader("If-None-Match");
      if (!etag.empty() &&
          etag == fmt::format("{:x}-{:x}", st.st_ino, st.st_mtime)) {
        response.SetHttpStatusCode(k304);
        break;
      }
      auto body = MakeHttpBody<StaticFileBody>(requestPath, st);
      if (!body->StaticFileAvailable()) {
        response.SetHttpStatusCode(k500);
        break;
      }
      response.SetBody(std::move(body));
    } while (0);
    if (response.GetHttpStatusCode() != k200 &&
        response.GetHttpStatusCode() != k304) {
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
