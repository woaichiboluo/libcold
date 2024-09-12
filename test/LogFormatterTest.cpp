#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <source_location>

#include "cold/log/LogFormatter.h"
#include "third_party/doctest.h"

using namespace Cold;

#define BufferToView(buffer) (std::string_view(buffer.begin(), buffer.end()))

LogMessage g_message = {LogLevel::TRACE, "8888", "main", "test", {}, {}, {},
                        "test logline"};

TEST_CASE("test one flag") {
  std::source_location location = std::source_location::current();
  g_message.location = location;
  g_message.baseName = "LogFormatterTest.cpp";
  LogFormatter formatter("", {});
  LogBuffer buffer;
  // thread id
  buffer.clear();
  formatter.SetPattern("%t");
  formatter.Format(g_message, buffer);
  CHECK(BufferToView(buffer) == "8888");
  // threadName
  buffer.clear();
  formatter.SetPattern("%N");
  formatter.Format(g_message, buffer);
  CHECK(BufferToView(buffer) == "main");
  // function name
  buffer.clear();
  formatter.SetPattern("%f");
  formatter.Format(g_message, buffer);
  CHECK(BufferToView(buffer) == location.function_name());
  // Filename
  buffer.clear();
  formatter.SetPattern("%F");
  formatter.Format(g_message, buffer);
  CHECK(BufferToView(buffer) == location.file_name());
  // basename
  buffer.clear();
  formatter.SetPattern("%b");
  formatter.Format(g_message, buffer);
  CHECK(BufferToView(buffer) ==
        "LogFormatterTest.cpp");  // basename take from Logger
  // fileline
  buffer.clear();
  formatter.SetPattern("%l");
  formatter.Format(g_message, buffer);
  CHECK(BufferToView(buffer) == std::to_string(location.line()));
  // logLevel
  buffer.clear();
  formatter.SetPattern("%L");
  buffer.clear();
  g_message.level = LogLevel::TRACE;
  formatter.Format(g_message, buffer);
  CHECK(BufferToView(buffer) == "TRACE");
  buffer.clear();
  g_message.level = LogLevel::DEBUG;
  formatter.Format(g_message, buffer);
  CHECK(BufferToView(buffer) == "DEBUG");
  buffer.clear();
  g_message.level = LogLevel::INFO;
  formatter.Format(g_message, buffer);
  CHECK(BufferToView(buffer) == "INFO ");
  buffer.clear();
  g_message.level = LogLevel::WARN;
  formatter.Format(g_message, buffer);
  CHECK(BufferToView(buffer) == "WARN ");
  buffer.clear();
  g_message.level = LogLevel::FATAL;
  formatter.Format(g_message, buffer);
  CHECK(BufferToView(buffer) == "FATAL");
  // Newline
  buffer.clear();
  formatter.SetPattern("%n");
  formatter.Format(g_message, buffer);
  CHECK(BufferToView(buffer) == "\n");
  // localTime
  buffer.clear();
  auto now = Time::Now();
  g_message.logTime = now;
  formatter.SetPattern("%T");
  formatter.Format(g_message, buffer);
  CHECK(buffer.size() == now.Dump(true).size());
  CHECK(BufferToView(buffer) == now.Dump(true));
  // logline Content
  buffer.clear();
  formatter.SetPattern("%c");
  formatter.Format(g_message, buffer);
  CHECK(BufferToView(buffer) == "test logline");
  // loggername
  buffer.clear();
  formatter.SetPattern("%m");
  formatter.Format(g_message, buffer);
  CHECK(BufferToView(buffer) == "test");
  // symbol %
  buffer.clear();
  formatter.SetPattern("%%");
  formatter.Format(g_message, buffer);
  CHECK(BufferToView(buffer) == "%");
}

TEST_CASE("test custom flag") {
  class CustomGFlag : public CustomFlagFormatter {
   public:
    CustomGFlag() = default;
    ~CustomGFlag() override = default;
    CustomFlagFormatterPtr Clone() const override {
      return std::make_unique<CustomGFlag>();
    }
    void Format(const LogMessage& message, LogBuffer& buffer) const override {
      std::string_view mes("G Flag");
      buffer.append(mes.begin(), mes.end());
    }
  };
  LogFormatter::FlagMap custom;
  custom['g'] = std::make_unique<CustomGFlag>();
  LogFormatter f("%g", std::move(custom));
  auto formatter = f.Clone();
  LogBuffer buffer;
  formatter->Format(g_message, buffer);
  CHECK(BufferToView(buffer) == "G Flag");
  buffer.clear();
  formatter->SetPattern("%N %g %n");
  formatter->Format(g_message, buffer);
  CHECK(BufferToView(buffer) == "main G Flag \n");
}

TEST_CASE("test multiple flag") {
  LogBuffer buffer;
  std::source_location location = std::source_location::current();
  g_message.location = location;
  g_message.baseName = "LogFormatterTest.cpp";
  g_message.threadId = "666";
  g_message.level = LogLevel::INFO;
  auto now = Time::Now();
  g_message.logTime = now;
  LogFormatter formatter("", {});
  formatter.SetPattern("%n %N %m %t %L %b %c %% %f%T%l");
  std::string expect =
      "\n main test 666 INFO  LogFormatterTest.cpp test logline % ";
  expect += location.function_name() + now.Dump(true) +
            std::to_string(location.line());
  formatter.Format(g_message, buffer);
  CHECK(BufferToView(buffer) == expect);
}