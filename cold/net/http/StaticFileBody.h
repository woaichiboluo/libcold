#ifndef NET_HTTP_STATICFILEBODY
#define NET_HTTP_STATICFILEBODY

#include <fcntl.h>
#include <unistd.h>

#include <cassert>
#include <chrono>
#include <filesystem>
#include <system_error>

#include "cold/net/IoAwaitable.h"
#include "cold/net/http/HttpCommon.h"
#include "cold/net/http/HttpResponse.h"
#include "cold/util/Config.h"
#include "cold/util/ScopeUtil.h"

namespace Cold::Net::Http {

class StaticFileBody : public HttpResponseBody {
 public:
  StaticFileBody(std::string requestPath, std::string docRoot,
                 HttpResponse* response)
      : requestPath_(std::move(requestPath)),
        docRoot_(std::move(docRoot)),
        response_(response),
        guard_([this]() {
          if (fd_ >= 0) close(fd_);
        }) {
    if (requestPath_.find("..") != std::string::npos) {
      response_->SetStatus(HttpStatus::NOT_FOUND);
      return;
    }
    if (requestPath_ == "/") requestPath_ = "/index.html";
    auto fullPath = docRoot_ + requestPath_;
    std::filesystem::path file(fullPath);
    auto status = std::filesystem::status(file);
    if (!std::filesystem::exists(status) ||
        !std::filesystem::is_regular_file(status) ||
        (status.permissions() & std::filesystem::perms::others_read) ==
            std::filesystem::perms::none) {
      response_->SetStatus(HttpStatus::NOT_FOUND);
      return;
    }
    std::error_code ec;
    fileSize_ = std::filesystem::file_size(file, ec);
    if (ec != std::error_code()) {
      response_->SetStatus(HttpStatus::INTERNAL_SERVER_ERROR);
      return;
    }
    fd_ = open(fullPath.data(), O_RDONLY);
    if (fd_ < 0) {
      response_->SetStatus(HttpStatus::INTERNAL_SERVER_ERROR);
      return;
    }
  }

  ~StaticFileBody() override = default;

  void SetRelatedHeaders(
      std::map<std::string, std::string>& headers) const override {
    if (fd_ == -1) return;
    static std::map<std::string, std::string> mimetypes = {
        {".html", "text/html"},
        {".htm", "text/html"},
        {".shtml", "text/html"},
        {".css", "text/css"},
        {".xml", "text/xml"},
        {".gif", "image/gif"},
        {".jpeg", "image/jpeg"},
        {".jpg", "image/jpeg"},
        {".js", "application/javascript"},
        {".atom", "application/atom+xml"},
        {".rss", "application/rss+xml"},
        {".mml", "text/mathml"},
        {".txt", "text/plain"},
        {".jad", "text/vnd.sun.j2me.app-descriptor"},
        {".wml", "text/vnd.wap.wml"},
        {".htc", "text/x-component"},
        {".png", "image/png"},
        {".tif", "image/tiff"},
        {".tiff", "image/tiff"},
        {".wbmp", "image/vnd.wap.wbmp"},
        {".ico", "image/x-icon"},
        {".jng", "image/x-jng"},
        {".bmp", "image/x-ms-bmp"},
        {".svg", "image/svg+xml"},
        {".svgz", "image/svg+xml"},
        {".webp", "image/webp"},
        {".woff", "font/woff"},
        {".woff2", "font/woff2"},
        {".jar", "application/java-archive"},
        {".war", "application/java-archive"},
        {".ear", "application/java-archive"},
        {".json", "application/json"},
        {".hqx", "application/mac-binhex40"},
        {".doc", "application/msword"},
        {".pdf", "application/pdf"},
        {".ps", "application/postscript"},
        {".eps", "application/postscript"},
        {".ai", "application/postscript"},
        {".rtf", "application/rtf"},
        {".m3u8", "application/vnd.apple.mpegurl"},
        {".xls", "application/vnd.ms-excel"},
        {".eot", "application/vnd.ms-fontobject"},
        {".ppt", "application/vnd.ms-powerpoint"},
        {".wmlc", "application/vnd.wap.wmlc"},
        {".kml", "application/vnd.google-earth.kml+xml"},
        {".kmz", "application/vnd.google-earth.kmz"},
        {".7z", "application/x-7z-compressed"},
        {".cco", "application/x-cocoa"},
        {".jardiff", "application/x-java-archive-diff"},
        {".jnlp", "application/x-java-jnlp-file"},
        {".run", "application/x-makeself"},
        {".pl", "application/x-perl"},
        {".pm", "application/x-perl"},
        {".prc", "application/x-pilot"},
        {".pdb", "application/x-pilot"},
        {".rar", "application/x-rar-compressed"},
        {".rpm", "application/x-redhat-package-manager"},
        {".sea", "application/x-sea"},
        {".swf", "application/x-shockwave-flash"},
        {".sit", "application/x-stuffit"},
        {".tcl", "application/x-tcl"},
        {".tk", "application/x-tcl"},
        {".der", "application/x-x509-ca-cert"},
        {".pem", "application/x-x509-ca-cert"},
        {".crt", "application/x-x509-ca-cert"},
        {".xpi", "application/x-xpinstall"},
        {".xhtml", "application/xhtml+xml"},
        {".xspf", "application/xspf+xml"},
        {".zip", "application/zip"},
        {".bin", "application/octet-stream"},
        {".exe", "application/octet-stream"},
        {".dll", "application/octet-stream"},
        {".deb", "application/octet-stream"},
        {".dmg", "application/octet-stream"},
        {".iso", "application/octet-stream"},
        {".img", "application/octet-stream"},
        {".msi", "application/octet-stream"},
        {".msp", "application/octet-stream"},
        {".msm", "application/octet-stream"},
        {".docx",
         "application/"
         "vnd.openxmlformats-officedocument.wordprocessingml.document"},
        {".xlsx",
         "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
        {".pptx",
         "application/"
         "vnd.openxmlformats-officedocument.presentationml.presentation"},
        {".mid", "audio/midi"},
        {".midi", "audio/midi"},
        {".kar", "audio/midi"},
        {".mp3", "audio/mpeg"},
        {".ogg", "audio/ogg"},
        {".m4a", "audio/x-m4a"},
        {".ra", "audio/x-realaudio"},
        {".3gpp", "video/3gpp"},
        {".3gp", "video/3gpp"},
        {".ts", "video/mp2t"},
        {".mp4", "video/mp4"},
        {".mpeg", "video/mpeg"},
        {".mpg", "video/mpeg"},
        {".mov", "video/quicktime"},
        {".webm", "video/webm"},
        {".flv", "video/x-flv"},
        {".m4v", "video/x-m4v"},
        {".mng", "video/x-mng"},
        {".asx", "video/x-ms-asf"},
        {".asf", "video/x-ms-asf"},
        {".wmv", "video/x-ms-wmv"},
        {".avi", "video/x-msvideo"}};
    headers["Content-Length"] = std::to_string(fileSize_);
    auto suffix = requestPath_.substr(requestPath_.find_last_of("."));
    if (mimetypes.contains(suffix)) {
      headers["Content-Type"] = mimetypes[suffix];
    } else {
      // default html
      headers["Content-Type"] = "text/html";
    }
    // add utf-8 encoding
    headers["Content-Type"] += "; charset=utf-8";
  }

  // SendComplete should return true else returnf false
  Base::Task<bool> Send(Net::TcpSocket& socket) override {
    static const int kReadTimeoutMs =
        Base::Config::GetGloablDefaultConfig().GetOrDefault(
            "/http/sendfile-timeout-ms", 15000);
    assert(response_->GetStatus() == HttpStatus::OK);
    assert(fd_ >= 0);
    off_t offset = 0;
    size_t alreadySend = 0;
    while (alreadySend < fileSize_ && socket.IsConnected()) {
      auto needSend = fileSize_ - alreadySend;
      auto ret = co_await IoTimeoutAwaitable(
          &socket.GetIoService(),
          SendFileAwaitable(&socket.GetIoService(), socket.NativeHandle(), fd_,
                            &offset, needSend),
          std::chrono::milliseconds(kReadTimeoutMs));
      if (ret < 0) break;
      alreadySend += static_cast<size_t>(ret);
    }
    co_return alreadySend == fileSize_;
  }

  // for debug
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

 private:
  std::string requestPath_;
  std::string docRoot_;
  HttpResponse* response_;
  int fd_ = -1;
  size_t fileSize_ = 0;
  Base::ScopeGuard<std::function<void()>> guard_;
};

}  // namespace Cold::Net::Http

#endif /* NET_HTTP_STATICFILEBODY */
