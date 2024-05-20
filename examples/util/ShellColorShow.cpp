#include <map>

#include "cold/util/ShellColor.h"
#include "third_party/fmt/include/fmt/core.h"

using namespace Cold::Base::Color;

void OnlyFrontColor() {
#define XX(value, name)                                                   \
  fmt::println("{}FrontColor::" #name "{}",                               \
               ColorControl(static_cast<FrontColor>(value)).ColorStart(), \
               ColorControl::ColorEnd());
  SHELL_FRONT_COLOR_MAP(XX)
#undef XX
}

void OnlyBackgroundColor() {
#define XX(value, name)                                                        \
  fmt::println("{}BackGroundColor::" #name "{}",                               \
               ColorControl(static_cast<BackGroundColor>(value)).ColorStart(), \
               ColorControl::ColorEnd());
  SHELL_BACKGROUND_COLOR_MAP(XX)
#undef XX
}

void FrontColorAndBackGroundColor() {
  std::map<FrontColor, std::string> front;
  std::map<BackGroundColor, std::string> back;
#define XX(value, name) front.emplace(static_cast<FrontColor>(value), #name);
  SHELL_FRONT_COLOR_MAP(XX)
#undef XX
#define XX(value, name) \
  back.emplace(static_cast<BackGroundColor>(value), #name);
  SHELL_BACKGROUND_COLOR_MAP(XX)
#undef XX
  for (const auto &[t1, m1] : front) {
    for (const auto &[t2, m2] : back) {
      ColorControl c(t1, t2);
      fmt::println("{} FrontColor::{} BackGroundColor::{} {}", c.ColorStart(),
                   m1, m2, c.ColorEnd());
    }
  }
}

void FrontColorAndBackGroundColorAndEmphasis() {
  std::map<FrontColor, std::string> front;
  std::map<BackGroundColor, std::string> back;
  std::map<Emphasis, std::string> emphasis;

#define XX(value, name) front.emplace(static_cast<FrontColor>(value), #name);
  SHELL_FRONT_COLOR_MAP(XX)
#undef XX
#define XX(value, name) \
  back.emplace(static_cast<BackGroundColor>(value), #name);
  SHELL_BACKGROUND_COLOR_MAP(XX)
#undef XX
#define XX(value, name) emphasis.emplace(static_cast<Emphasis>(value), #name);
  SHELL_EMPHASIS_MAP(XX)
#undef XX

  for (const auto &[t1, m1] : front) {
    for (const auto &[t2, m2] : back) {
      for (const auto &[t3, m3] : emphasis) {
        if (t3 == Emphasis::kReset) continue;
        ColorControl c(t1, t2, t3);
        fmt::println("{} FrontColor::{} BackGroundColor::{} Emphasis::{} {}",
                     c.ColorStart(), m1, m2, m3, c.ColorEnd());
      }
    }
  }
}

int main() {
  OnlyFrontColor();
  OnlyBackgroundColor();
  FrontColorAndBackGroundColor();
  FrontColorAndBackGroundColorAndEmphasis();
}