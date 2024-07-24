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
      : servletRouterRoot_(std::make_unique<ServletNode>()),
        filterRouterRoot_(std::make_unique<FilterNode>()) {}

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
  class ServletNode {
   public:
    ServletNode() = default;
    ~ServletNode() = default;

    ServletNode(const ServletNode&) = delete;
    ServletNode& operator=(const ServletNode&) = delete;

    void Insert(std::string_view url, std::unique_ptr<HttpServlet> ptr) {
      auto parts = Base::SplitToViews(url, "/", true);
      auto last = this;
      for (const auto& part : parts) {
        auto p = std::string(part);
        if (last->children_.find(p) == last->children_.end()) {
          auto node = std::make_unique<ServletNode>();
          last->children_.insert({p, std::move(node)});
        }
        last = last->children_[p].get();
      }
      assert(last);
      last->param_ = std::move(ptr);
    }

    HttpServlet* TryMatch(std::string_view url) {
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

   private:
    std::map<std::string, std::unique_ptr<ServletNode>> children_;
    std::unique_ptr<HttpServlet> param_;
  };

  struct FilterNode {
   public:
    FilterNode() = default;
    ~FilterNode() = default;

    FilterNode(const FilterNode&) = delete;
    FilterNode& operator=(const FilterNode&) = delete;

    void Insert(std::string_view url, std::unique_ptr<HttpFilter> ptr) {
      auto parts = Base::SplitToViews(url, "/", true);
      auto last = this;
      for (const auto& part : parts) {
        auto p = std::string(part);
        if (last->children_.find(p) == last->children_.end()) {
          auto node = std::make_unique<FilterNode>();
          last->children_.insert({p, std::move(node)});
        }
        last = last->children_[p].get();
      }
      assert(last);
      last->params_.push_back(std::move(ptr));
    }

    std::vector<HttpFilter*> MatchChain(std::string_view url) {
      std::vector<HttpFilter*> chains;
      auto parts = Base::SplitToViews(url, "/", true);
      auto last = this;
      for (const auto& part : parts) {
        auto p = std::string(part);
        if (last->children_.find("**") != last->children_.end()) {
          for (const auto& filter : last->children_["**"]->params_) {
            chains.push_back(filter.get());
          }
        }
        if (last->children_.find(p) == last->children_.end()) {
          return chains;
        }
        last = last->children_[p].get();
      }
      assert(last);
      if (!last->params_.empty()) {
        for (const auto& filter : last->params_) {
          chains.push_back(filter.get());
        }
      }
      return chains;
    }

   private:
    std::map<std::string, std::unique_ptr<FilterNode>> children_;
    std::vector<std::unique_ptr<HttpFilter>> params_;
  };

  using ServletRouterRoot = std::unique_ptr<ServletNode>;
  using FilterRouterRoot = std::unique_ptr<FilterNode>;

  ServletRouterRoot servletRouterRoot_;
  FilterRouterRoot filterRouterRoot_;
};

}  // namespace Cold::Net::Http

#endif /* NET_HTTP_ROUTER */
