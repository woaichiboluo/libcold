#ifndef COLD_HTTP_ROUTER
#define COLD_HTTP_ROUTER

#include <cassert>
#include <memory>
#include <string_view>
#include <vector>

namespace Cold::Http {

class HttpFilter;
class HttpServlet;

template <typename T>
class Router {
 public:
  Router() = default;
  ~Router() = default;

  Router(const Router&) = delete;
  Router& operator=(const Router&) = delete;

  void AddRoute(std::string url, std::shared_ptr<T> nodeValue) {
    assert(!url.empty() && url[0] == '/');
    Node node(std::move(url), std::move(nodeValue));
    nodes_.push_back(std::move(node));
  }

  T* MatchOne(std::string_view url) {
    auto views = SplitToViews(url);
    T* ret = nullptr;
    for (const auto& node : nodes_) {
      auto state = UrlMatch(views, node.pattern);
      if (state == kFullMatch) {
        return node.value_.get();
      } else if (state == kFuzzyMatch) {
        ret = node.value_.get();
      }
    }
    return ret;
  }

  std::vector<T*> MatchChains(std::string_view url) {
    std::vector<T*> chains;
    auto views = SplitToViews(url);
    for (const auto& node : nodes_) {
      auto state = UrlMatch(views, node.pattern);
      if (state == kFullMatch || state == kFuzzyMatch) {
        chains.push_back(node.value_.get());
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

  struct Node {
    Node() = default;
    ~Node() = default;
    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;
    Node(std::string u, std::shared_ptr<T> p)
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
    std::shared_ptr<T> value_;
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

  std::vector<Node> nodes_;
};

}  // namespace Cold::Http

#endif /* COLD_HTTP_ROUTER */
