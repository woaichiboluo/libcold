#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "cold/util/Config.h"
#include "third_party/doctest.h"

const char* g_raw = R"({
    "group1": {
        "config1": "1",
        "config2": 1,
        "config3": false
    },
    "group2": {
        "config1": "2",
        "config2": 2,
        "config3": true
    }
})";

const char* g_raw_updated = R"({
    "group1": {
        "config1": "1",
        "config2": 1,
        "config3": false
    },
    "group2": {
        "config1": "2",
        "config2": 2,
        "config3": true
    },
    "group3": {
        "config1": "3",
        "config2": 3,
        "config3": false
    }
})";

TEST_CASE("test read") {
  std::ofstream out("ConfigTest.json");
  out << g_raw;
  out.close();
  Cold::Base::Config config("ConfigTest.json");
  CHECK(!config.GetConfigJson().empty());
  CHECK(config.GetConfigJson().size() == 2);
  CHECK(config.Contains("/group1/config1"));
  CHECK(config.GetConfig("/group1/config1") == "1");
  CHECK(config.Contains("/group1/config2"));
  CHECK(config.GetConfig("/group1/config2") == 1);
  CHECK(config.Contains("/group1/config3"));
  CHECK(config.GetConfig("/group1/config3") == false);
  CHECK(config.Contains("/group2/config1"));
  CHECK(config.GetConfig("/group2/config1") == "2");
  CHECK(config.Contains("/group2/config2"));
  CHECK(config.GetConfig("/group2/config2") == 2);
  CHECK(config.Contains("/group2/config3"));
  CHECK(config.GetConfig("/group2/config3") == true);
}

TEST_CASE("test save") {
  Cold::Base::Config config("ConfigTest.json");
  config.SetConfig("/group3/config1", "3");
  config.GetOrDefault("/group3/config2", 3);
  config.SetConfig("/group3/config3", false);
  config.SaveConfig();
  std::ifstream in("ConfigTest.json");
  std::string buf{std::istreambuf_iterator<char>(in),
                  std::istreambuf_iterator<char>()};
  CHECK(buf == g_raw_updated);
}