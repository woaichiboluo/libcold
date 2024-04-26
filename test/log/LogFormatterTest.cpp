#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <source_location>

#include "cold/log/LogCommon.h"
#include "cold/log/LogFormatter.h"
#include "cold/log/Logger.h"
#include "cold/thread/Thread.h"
#include "third_party/doctest.h"

using namespace Cold;

#define BufferToView(buffer) (std::string_view(buffer.begin(), buffer.end()))

Base::LogMessage g_message = {Base::LogLevel::TRACE,
                              Base::ThisThread::ThreadIdStr(),
                              Base::ThisThread::ThreadName(),
                              "test",
                              {},
                              {},
                              {},
                              "test logline"};

TEST_CASE("test one flag") {
  constexpr Base::LocationWrapper wrapper;
  g_message.location = wrapper.location;
  g_message.baseName = wrapper.baseName;
  Base::LogFormatter formatter;
  Base::LogBuffer buffer;
  // thread id
  buffer.clear();
  formatter.SetPattern("%t");
  formatter.Format(g_message, buffer);
  CHECK(BufferToView(buffer) == Base::ThisThread::ThreadIdStr());
  // threadName
  buffer.clear();
  formatter.SetPattern("%N");
  formatter.Format(g_message, buffer);
  CHECK(BufferToView(buffer) == Base::ThisThread::ThreadName());
  // function name
  buffer.clear();
  formatter.SetPattern("%f");
  formatter.Format(g_message, buffer);
  CHECK(BufferToView(buffer) == wrapper.location.function_name());
  // Filename
  buffer.clear();
  formatter.SetPattern("%F");
  formatter.Format(g_message, buffer);
  CHECK(BufferToView(buffer) == wrapper.location.file_name());
  // basename
  buffer.clear();
  formatter.SetPattern("%b");
  formatter.Format(g_message, buffer);
  CHECK(BufferToView(buffer) == wrapper.baseName);  // basename take from Logger
  // fileline
  buffer.clear();
  formatter.SetPattern("%l");
  formatter.Format(g_message, buffer);
  CHECK(BufferToView(buffer) == std::to_string(wrapper.location.line()));
  // logLevel
  buffer.clear();
  formatter.SetPattern("%L");
  buffer.clear();
  g_message.level = Base::LogLevel::TRACE;
  formatter.Format(g_message, buffer);
  CHECK(BufferToView(buffer) == "TRACE");
  buffer.clear();
  g_message.level = Base::LogLevel::DEBUG;
  formatter.Format(g_message, buffer);
  CHECK(BufferToView(buffer) == "DEBUG");
  buffer.clear();
  g_message.level = Base::LogLevel::INFO;
  formatter.Format(g_message, buffer);
  CHECK(BufferToView(buffer) == "INFO ");
  buffer.clear();
  g_message.level = Base::LogLevel::WARN;
  formatter.Format(g_message, buffer);
  CHECK(BufferToView(buffer) == "WARN ");
  buffer.clear();
  g_message.level = Base::LogLevel::FATAL;
  formatter.Format(g_message, buffer);
  CHECK(BufferToView(buffer) == "FATAL");
  // Newline
  buffer.clear();
  formatter.SetPattern("%n");
  formatter.Format(g_message, buffer);
  CHECK(BufferToView(buffer) == "\n");
  // localTime
  buffer.clear();
  auto now = Base::Time::Now();
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
  class CustomGFlag : public Base::CustomFlagFormatter {
   public:
    CustomGFlag() = default;
    ~CustomGFlag() override = default;
    CustomFlagFormatterPtr Clone() const override {
      return std::make_unique<CustomGFlag>();
    }
    void Format(const Base::LogMessage& message,
                Base::LogBuffer& buffer) const override {
      std::string_view mes("G Flag");
      buffer.append(mes.begin(), mes.end());
    }
  };
  Base::LogFormatter::FlagMap custom;
  custom['g'] = std::make_unique<CustomGFlag>();
  Base::LogFormatter f("%g", std::move(custom));
  auto formatter = f.Clone();
  Base::LogBuffer buffer;
  formatter->Format(g_message, buffer);
  CHECK(BufferToView(buffer) == "G Flag");
  buffer.clear();
  formatter->SetPattern("%N %g %n");
  formatter->Format(g_message, buffer);
  CHECK(BufferToView(buffer) == "main G Flag \n");
}

TEST_CASE("test multiple flag") {
  Base::LogBuffer buffer;
  constexpr Base::LocationWrapper wrapper;
  g_message.location = wrapper.location;
  g_message.baseName = wrapper.baseName;
  g_message.threadId = "666";
  g_message.level = Base::LogLevel::INFO;
  auto now = Base::Time::Now();
  g_message.logTime = now;
  Base::LogFormatter formatter;
  formatter.SetPattern("%n %N %m %t %L %b %c %% %f%T%l");
  std::string expect =
      "\n main test 666 INFO  LogFormatterTest.cpp test logline % ";
  expect += wrapper.location.function_name() + now.Dump(true) +
            std::to_string(wrapper.location.line());
  formatter.Format(g_message, buffer);
  CHECK(BufferToView(buffer) == expect);
}

// These case will abort program
// TEST_CASE("test bad pattern") {
//   Base::LogFormatter formatter;
//   formatter.SetPattern("%1");

//   formatter.SetPattern("%");

//   formatter.SetPattern("%t %T %");

//   formatter.SetPattern("%%% %t %T ");

//   formatter.SetPattern("% %");

//   formatter.SetPattern("%t %c %");

//   formatter.SetPattern("% %t %c %n");
// }