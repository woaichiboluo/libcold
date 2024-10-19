#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cold/http/HttpFilter.h"
#include "cold/http/HttpServlet.h"
#include "cold/http/Router.h"
#include "third_party/doctest.h"

using namespace Cold::Http;

class MyServlet : public HttpServlet {
 public:
  MyServlet(int v) : v_(v) {}

  int GetV() const { return v_; }

 private:
  int v_;
};

TEST_CASE("basic match") {
  Router router;
  router.AddFilter("/hello", std::make_unique<HttpFilter>());
  CHECK(router.MatchFilterChain("/hello").size() == 1);
  router.AddFilter("/**", std::make_unique<HttpFilter>());
  CHECK(router.MatchFilterChain("/hello").size() == 2);
  router.AddFilter("/*/world", std::make_unique<HttpFilter>());
  CHECK(router.MatchFilterChain("/dsjflkasdfjlk/world").size() == 2);
  CHECK(router.MatchFilterChain("/").size() == 1);
  CHECK(router.MatchFilterChain("//world").size() == 1);

  router.AddServlet("/hello", std::make_unique<MyServlet>(1));
  CHECK(dynamic_cast<MyServlet*>(router.MatchServlet("/hello"))->GetV() == 1);
  router.AddServlet("/**", std::make_unique<MyServlet>(2));
  CHECK(dynamic_cast<MyServlet*>(router.MatchServlet("/hello"))->GetV() == 1);
  CHECK(dynamic_cast<MyServlet*>(router.MatchServlet("/any"))->GetV() == 2);

  router.AddServlet("/hello/*/world", std::make_unique<MyServlet>(3));
  router.AddServlet("/hello/hello/world", std::make_unique<MyServlet>(4));
  CHECK(dynamic_cast<MyServlet*>(router.MatchServlet("/hello/any/world"))
            ->GetV() == 3);
  CHECK(dynamic_cast<MyServlet*>(router.MatchServlet("/hello/hello/world"))
            ->GetV() == 4);
}