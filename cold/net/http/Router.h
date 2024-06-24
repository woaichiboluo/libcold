#ifndef NET_HTTP_ROUTER
#define NET_HTTP_ROUTER

#include <cassert>
#include <map>
#include <memory>
#include <string>

#include "cold/net/http/HttpFilter.h"
#include "cold/net/http/HttpServlet.h"
#include "cold/util/StringUtils.h"

namespace Cold::Net::Http {

class Router {
 public:
  using FilterPtr = std::unique_ptr<HttpFilter>;
  using ServletPtr = std::unique_ptr<HttpServlet>;

  Router()
      : servletRouterRoot_(std::make_unique<TrieNode<HttpServlet>>()),
        filterRouterRoot_(std::make_unique<TrieNode<HttpFilter>>()) {}

  ~Router() = default;

  Router(const Router&) = delete;
  Router& operator=(const Router&) = delete;

  void AddRoute(std::string_view url, FilterPtr filter) {
    filterRouterRoot_->Insert(url, std::move(filter));
  }

  void AddRoute(std::string_view url, ServletPtr servlet) {
    servletRouterRoot_->Insert(url, std::move(servlet));
  }

  std::vector<HttpFilter*> MatchFilters(std::string_view url) {
    return filterRouterRoot_->MatchChain(url);
  }

  HttpServlet* MatchServlet(std::string_view url) {
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

    void Insert(std::string_view url, std::unique_ptr<T> ptr) {
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
      last->param_ = std::move(ptr);
    }

    T* TryMatch(std::string_view url) {
      auto parts = Base::SplitToViews(url, "/", true);
      auto last = this;
      for (const auto& part : parts) {
        auto p = std::string(part);
        if (last->children_.find(p) == last->children_.end()) {
          auto it = last->children_.find("**");
          return it == last->children_.end() ? nullptr
                                             : it->second->param_.get();
        } else {
          last = last->children_[p].get();
        }
      }
      assert(last);
      return last->param_.get();
    }

    std::vector<T*> MatchChain(std::string_view url) {
      std::vector<T*> chains;
      auto parts = Base::SplitToViews(url, "/", true);
      auto last = this;
      for (const auto& part : parts) {
        auto p = std::string(part);
        if (last->children_.find("**") != last->children_.end()) {
          chains.push_back(last->children_["**"]->param_.get());
        }
        if (last->children_.find(p) == last->children_.end()) {
          return chains;
        }
        last = last->children_[p].get();
      }
      assert(last);
      if (last->param_) chains.push_back(last->param_.get());
      return chains;
    }

   private:
    std::map<std::string, std::unique_ptr<TrieNode>> children_;
    std::unique_ptr<T> param_;
  };

  using ServletRouterRoot = std::unique_ptr<TrieNode<HttpServlet>>;
  using FilterRouterRoot = std::unique_ptr<TrieNode<HttpFilter>>;

  ServletRouterRoot servletRouterRoot_;
  FilterRouterRoot filterRouterRoot_;
};

}  // namespace Cold::Net::Http

#endif /* NET_HTTP_ROUTER */
