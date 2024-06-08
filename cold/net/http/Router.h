#ifndef NET_HTTP_ROUTER
#define NET_HTTP_ROUTER

#include <cassert>
#include <map>
#include <memory>
#include <string>

#include "cold/net/http/Servlet.h"
#include "cold/util/StringUtils.h"

namespace Cold::Net::Http {

class Filter {};

class Router {
 public:
  using FilterPtr = std::shared_ptr<Filter>;
  using ServletPtr = std::shared_ptr<HttpServlet>;

  Router()
      : servletRouterRoot_(std::make_unique<TrieNode<HttpServlet>>()),
        filterRouterRoot_(std::make_unique<TrieNode<Filter>>()) {}

  ~Router() = default;

  Router(const Router&) = delete;
  Router& operator=(const Router&) = delete;

  void AddRoute(std::string_view url, FilterPtr filter) {
    filterRouterRoot_->Insert(url, filter);
  }

  void AddRoute(std::string_view url, ServletPtr servlet) {
    servletRouterRoot_->Insert(url, servlet);
  }

  FilterPtr MatchFilter(std::string_view url) {
    return filterRouterRoot_->TryMatch(url);
  }

  ServletPtr MatchServlet(std::string_view url) {
    return servletRouterRoot_->TryMatch(url);
  }

 private:
  template <typename T>
  struct TrieNode {
   public:
    TrieNode() = default;
    ~TrieNode() = default;

    TrieNode(const TrieNode&) = delete;
    TrieNode& operator=(const TrieNode&) = delete;

    void Insert(std::string_view url, std::shared_ptr<T> ptr) {
      auto parts = Base::SplitToViews(url, "/", true);
      auto last = this;
      for (const auto& part : parts) {
        auto p = std::string(part);
        if (last->children_.find(p) == last->children_.end()) {
          auto node = std::make_unique<TrieNode>();
          last->children_.insert({p, std::move(node)});
        }
        last = last->children_[p].get();
      }
      assert(last);
      last->param_ = ptr;
    }

    std::shared_ptr<T> TryMatch(std::string_view url) {
      auto parts = Base::SplitToViews(url, "/", true);
      auto last = this;
      for (const auto& part : parts) {
        auto p = std::string(part);
        if (last->children_.find(p) == last->children_.end()) {
          auto it = last->children_.find("**");
          return it == last->children_.end() ? nullptr
                                             : it->second.get()->param_;
        } else {
          last = last->children_[p].get();
        }
      }
      assert(last);
      return last->param_;
    }

   private:
    std::map<std::string, std::unique_ptr<TrieNode>> children_;
    std::shared_ptr<T> param_;
  };

  using ServletRouterRoot = std::unique_ptr<TrieNode<HttpServlet>>;
  using FilterRouterRoot = std::unique_ptr<TrieNode<Filter>>;

  ServletRouterRoot servletRouterRoot_;
  FilterRouterRoot filterRouterRoot_;
};

}  // namespace Cold::Net::Http

#endif /* NET_HTTP_ROUTER */
