#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "cold/net/http/Router.h"
#include "third_party/doctest.h"

using namespace Cold;

TEST_CASE("basic") {
  // Net::Http::Router router;
  // auto f1 = new Net::Http::Filter();
  // router.AddRoute("/hello/world", std::unique_ptr<Net::Http::Filter>(f1));
  // auto p = router.MatchFilter("/hello/world");
  // CHECK(p);
  // CHECK(p == f1);
  // auto p1 = router.MatchFilter("/hello/world1");
  // CHECK(!p1);
  // auto f2 = new Net::Http::Filter();
  // router.AddRoute("/hello/**", std::unique_ptr<Net::Http::Filter>(f2));
  // auto p2 = router.MatchFilter("/hello/world");
  // CHECK(p2);
  // CHECK(p2 == f1);
  // auto p3 = router.MatchFilter("/hello/xxsad1asdasd");
  // CHECK(p3);
  // CHECK(p3 == f2);
  // auto p4 = router.MatchFilter("/hello/xxsad1asdasd/fasdfasd");
  // CHECK(p4);
  // CHECK(p4 == f2);
  // auto p5 = router.MatchFilter("/hello");
  // CHECK(!p5);
}