#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <memory>

#include "cold/coro/Task.h"
#include "third_party/doctest.h"

using Cold::Base::Task;

TEST_CASE("Test Resume Task") {
  int value = 0;
  auto task = [](int &v) -> Task<> {
    v = 100;
    co_return;
  }(value);
  CHECK(value == 0);
  CHECK(task.Done() == false);
  task.Resume();
  CHECK(value == 100);
  CHECK(task.Done());
}

int g_value1 = 0;
int g_value2 = 0;
int g_value3 = 0;

Task<> Co1() {
  g_value1 = 111;
  co_return;
}

Task<> Co2() {
  g_value2 = 222;
  CHECK(g_value1 == 0);
  CHECK(g_value3 == 333);
  co_await Co1();
  CHECK(g_value1 == 111);
  CHECK(g_value2 == 222);
  CHECK(g_value3 == 333);
  co_return;
}

Task<> Co3() {
  g_value3 = 333;
  CHECK(g_value1 == 0);
  CHECK(g_value2 == 0);
  co_await Co2();
  CHECK(g_value1 == 111);
  CHECK(g_value2 == 222);
  CHECK(g_value3 == 333);
  co_return;
}

TEST_CASE("Test Nested tasks") {
  auto coro = []() -> Task<> { co_await Co3(); }();
  CHECK(coro.Done() == false);
  coro.Resume();
  CHECK(coro.Done());
}

TEST_CASE("Test move task") {
  int value = 0;
  auto coro = [](int *v) -> Task<> {
    *v = 666;
    co_return;
  }(&value);
  auto c = std::move(coro);
  coro = std::move(c);
  c = std::move(coro);
  CHECK(c.Done() == false);
  CHECK(value == 0);
  c.Resume();
  CHECK(c.Done());
  CHECK(value == 666);
}

TEST_CASE("Test task return value") {
  auto intCoro = []() -> Task<int> { co_return 888; }();
  auto rvalueCoro = []() -> Task<std::unique_ptr<int>> {
    co_return std::make_unique<int>(100);
  }();
  auto strCoro = []() -> Task<std::string> { co_return "Hello World"; }();
  auto strviewCoro = []() -> Task<std::string_view> {
    co_return "Hello World";
  }();
  auto vectorCoro = []() -> Task<std::vector<size_t>> {
    co_return {1, 2, 3, 4, 5};
  }();
  /*
  //错误的使用方法 捕获的变量存放与lambda中.当coro resume时,捕获的变量已经析构
  auto coro = [&]() -> Task<> {
    auto v1 = co_await intCoro;
    CHECK(v1 == 888);
  }();
  */
  auto coro = [](Task<int> a, Task<std::unique_ptr<int>> b, Task<std::string> c,
                 Task<std::string_view> d,
                 Task<std::vector<size_t>> e) -> Task<> {
    auto v1 = co_await a;
    CHECK(a.Done());
    CHECK(v1 == 888);
    auto v2 = co_await b;
    CHECK(b.Done());
    CHECK(*v2 == 100);
    auto v3 = co_await c;
    CHECK(c.Done());
    CHECK(v3 == "Hello World");
    auto v4 = co_await d;
    CHECK(d.Done());
    CHECK(v4 == "Hello World");
    auto v5 = co_await e;
    CHECK(e.Done());
    CHECK(v5 == std::vector<size_t>{1, 2, 3, 4, 5});
  }(std::move(intCoro), std::move(rvalueCoro), std::move(strCoro),
                                              std::move(strviewCoro),
                                              std::move(vectorCoro));
  //这时的调用是安全的 所有的Task都被保存到了协程帧里面
  coro.Resume();
  CHECK(coro.Done());
}