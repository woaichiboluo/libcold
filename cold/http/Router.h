#ifndef COLD_HTTP_ROUTER
#define COLD_HTTP_ROUTER

#include <cassert>
#include <memory>
#include <string_view>
#include <vector>

namespace Cold::Http {

class HttpFilter;
class HttpServlet;

class Router {
 public:
  Router() = default;
  ~Router() = default;

  void AddServlet(std::string url, std::unique_ptr<HttpServlet> servlet) {
    assert(!url.empty() && url[0] == '/');
    Node<HttpServlet> node(std::move(url), std::move(servlet));
    servlets_.push_back(std::move(node));
  }

  void AddFilter(std::string url, std::unique_ptr<HttpFilter> filter) {
    assert(!url.empty() && url[0] == '/');
    Node<HttpFilter> node(std::move(url), std::move(filter));
    filters_.push_back(std::move(node));
  }

  HttpServlet* MatchServlet(std::string_view url) {
    auto views = SplitToViews(url);
    HttpServlet* ret = nullptr;
    for (const auto& node : servlets_) {
      auto state = UrlMatch(views, node.pattern);
      if (state == kFullMatch) {
        return node.value_.get();
      } else if (state == kFuzzyMatch) {
        ret = node.value_.get();
      }
    }
    return ret;
  }

  std::vector<HttpFilter> MatchFilterChain(std::string_view url) {
    std::vector<HttpFilter> chains;
    auto views = SplitToViews(url);
    for (const auto& node : filters_) {
      auto state = UrlMatch(views, node.pattern);
      if (state == kFullMatch || state == kFuzzyMatch) {
        chains.push_back(*node.value_);
      }
    }
    return chains;
  }

 private:
  enum MatchState {
    kFullMatch,
    kFuzzyMatch,
    kNotMatch,
  };

  static MatchState UrlMatch(std::vector<std::string_view> url,
                             std::vector<std::string_view> pattern) {
    auto n = url.size(), m = pattern.size();
    size_t i = 0, j = 0;
    MatchState state = kFullMatch;
    while (i < n && j < m) {
      if (pattern[j] == "/**") {
        return kFuzzyMatch;
      }
      if (url[i] == pattern[j]) {
        ++j;
        ++i;
      } else if ((pattern[j] == "/*" && url[i].size() > 1)) {
        state = kFuzzyMatch;
        ++j;
        ++i;
      } else {
        return kNotMatch;
      }
    }
    return (i >= n && j >= m) ? state : kNotMatch;
  }

  template <typename T>
  struct Node {
    Node() = default;
    ~Node() = default;
    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;
    Node(std::string u, std::unique_ptr<T> p)
        : url(std::move(u)), pattern(SplitToViews(url)), value_(std::move(p)) {}

    Node(Node&& other)
        : url(std::move(other.url)),
          pattern(SplitToViews(url)),
          value_(std::move(other.value_)) {}

    Node& operator=(Node&& other) {
      if (this == &other) return *this;
      url = std::move(other.url);
      pattern = SplitToViews(url);
      value_ = std::move(other.value_);
      return *this;
    }

    std::string url;
    std::vector<std::string_view> pattern;
    std::unique_ptr<T> value_;
  };

  static std::vector<std::string_view> SplitToViews(std::string_view str) {
    std::vector<std::string_view> views;
    size_t start = 0;
    for (size_t i = 1; i < str.size(); ++i) {
      if (str[i] == '/') {
        views.push_back(str.substr(start, i - start));
        start = i;
      }
    }
    views.push_back(str.substr(start));
    return views;
  }

  std::vector<Node<HttpServlet>> servlets_;
  std::vector<Node<HttpFilter>> filters_;
};

}  // namespace Cold::Http

#endif /* COLD_HTTP_ROUTER */
