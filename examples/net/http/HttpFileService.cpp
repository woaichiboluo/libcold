#include <filesystem>

#include "cold/Cold-Http.h"
using namespace Cold;
using namespace Cold::Http;

class DirectoryServlet : public HttpServlet {
 public:
  DirectoryServlet(std::filesystem::path cur, std::string parent,
                   std::string name)
      : cur_(std::move(cur)),
        parent_(std::move(parent)),
        name_(std::move(name)) {}
  ~DirectoryServlet() override = default;

  void DoService(HttpRequest& request, HttpResponse& response) override {
    auto body = MakeHttpBody<HtmlTextBody>();
    body->Append(
        fmt::format("<html><head><title>{}</title></head><body>", name_));
    body->Append(fmt::format("<h1>Cur location: {}</h1>", name_));
    body->Append("<ul>");
    if (!parent_.empty()) {
      auto href = parent_ != "/" ? parent_.substr(0, parent_.size() - 1) : "/";
      body->Append(fmt::format("<li><a href='{}'>{}</a></li>", href, ".."));
    }
    for (const auto& entry : std::filesystem::directory_iterator(cur_)) {
      // get entry base name
      auto base = entry.path().filename().string();
      auto href = name_ + base;
      bool isDir = std::filesystem::is_directory(entry.path());
      body->Append(fmt::format("<li><a href='{}'>{}{}</a></li>", href,
                               isDir ? "/" : "", base));
    }
    body->Append("</ul>");
    body->Append("</body></html>");
    response.SetBody(std::move(body));
  }

 private:
  std::filesystem::path cur_;
  std::string parent_;
  std::string name_;
};

int main(int argc, char** argv) {
  if (argc < 2) {
    fmt::println("Usage: {} <path>", argv[0]);
    return 1;
  }
  // check if the path is a directory
  std::filesystem::path p(argv[1]);
  if (!std::filesystem::is_directory(p)) {
    fmt::println("Error: {} is not a directory", p.string());
    return 1;
  }

  // 0 now 1 parent 2 name
  using DirectoryServletParam =
      std::tuple<std::filesystem::path, std::string, std::string>;
  std::vector<DirectoryServletParam> dirs;
  auto d = [&](auto&& dfs, const std::filesystem::path& now,
               const std::string& parent, std::string name) -> void {
    dirs.push_back({now, parent, name});
    for (const auto& entry : std::filesystem::directory_iterator(now)) {
      if (std::filesystem::is_directory(entry.path())) {
        dfs(dfs, entry.path(), name,
            name + entry.path().filename().string() + "/");
      }
    }
  };
  d(d, p, "", "/");

  IpAddress addr(8080);
  HttpServer server(addr, 4);
  server.SetHost("localhost");
  for (const auto& [now, parent, name] : dirs) {
    // remove last '/'
    auto pattern = name.size() > 1 ? name.substr(0, name.size() - 1) : "/";
    server.AddServlet(pattern,
                      std::make_unique<DirectoryServlet>(now, parent, name));
  }
  server.AddServlet("/**", std::make_unique<StaticFileServlet>(p));
  server.Start();
  return 0;
}