#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cold/http/HttpFilter.h"
#include "cold/http/HttpServlet.h"
#include "cold/http/Router.h"
#include "third_party/doctest.h"

using namespace Cold::Http;

TEST_CASE("basic match") {
  Router<HttpFilter> router;
  router.AddRoute("/hello", std::make_shared<HttpFilter>());
  CHECK(router.MatchChains("/hello").size() == 1);
  router.AddRoute("/**", std::make_shared<HttpFilter>());
  CHECK(router.MatchChains("/hello").size() == 2);
  router.AddRoute("/*/world", std::make_unique<HttpFilter>());
  CHECK(router.MatchChains("/dsjflkasdfjlk/world").size() == 2);
  CHECK(router.MatchChains("/").size() == 1);
  CHECK(router.MatchChains("//world").size() == 1);

  Router<HttpServlet> router2;
  auto servlet1 = std::make_shared<HttpServlet>();
  auto servlet2 = std::make_shared<HttpServlet>();
  auto servlet3 = std::make_shared<HttpServlet>();
  auto servlet4 = std::make_shared<HttpServlet>();
  router2.AddRoute("/hello", servlet1);
  CHECK(router2.MatchOne("/hello") == servlet1.get());
  router2.AddRoute("/**", servlet2);
  CHECK(router2.MatchOne("/hello") == servlet1.get());
  CHECK(router2.MatchOne("/any") == servlet2.get());

  router2.AddRoute("/hello/*/world", servlet3);
  router2.AddRoute("/hello/hello/world", servlet4);
  CHECK(router2.MatchOne("/hello/any/world") == servlet3.get());
  CHECK(router2.MatchOne("/hello/hello/world") == servlet4.get());
}