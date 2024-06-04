#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "cold/net/http/Router.h"
#include "third_party/doctest.h"

using namespace Cold;

TEST_CASE("basic") {
  Net::Router router;
  auto f1 = std::make_shared<Net::Filter>();
  router.AddRoute("/hello/world", f1);
  auto p = router.MatchFilter("/hello/world");
  CHECK(p);
  CHECK(p.get() == f1.get());
  auto p1 = router.MatchFilter("/hello/world1");
  CHECK(!p1);
  auto f2 = std::make_shared<Net::Filter>();
  router.AddRoute("/hello/**", f2);
  auto p2 = router.MatchFilter("/hello/world");
  CHECK(p2);
  CHECK(p2.get() == f1.get());
  auto p3 = router.MatchFilter("/hello/xxsad1asdasd");
  CHECK(p3);
  CHECK(p3.get() == f2.get());
  auto p4 = router.MatchFilter("/hello/xxsad1asdasd/fasdfasd");
  CHECK(p4);
  CHECK(p4.get() == f2.get());
  auto p5 = router.MatchFilter("/hello");
  CHECK(!p5);
}