#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "cold/net/http/HttpRequestParser.h"
#include "third_party/doctest.h"
#include "third_party/fmt/include/fmt/core.h"

TEST_CASE("basic") {
  const char* r1 = "GET / HTTP/1.1\r\n\r\n";
  Cold::Net::Http::HttpRequestParser parser;
  CHECK(parser.Parse(r1, strlen(r1)));
  CHECK(parser.HasRequest());
  auto request1 = parser.TakeRequest();
  CHECK(request1.GetMethod() == "GET");
  CHECK(request1.GetVersion() == "HTTP/1.1");
  CHECK(request1.GetAllHeader().empty());
  CHECK(request1.GetBody().empty());

  const char* r2 =
      "GET /someaddr?key1=value1#frag HTTP/1.1\r\n"
      "Host: www.xxxx.com:80\r\n"
      "Connection: close\r\n"
      "Accept: application/json, text/javascript, */*; q=0.01\r\n"
      "X-Requested-With: XMLHttpRequest\r\n"
      "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
      "AppleWebKit/537.36 (KHTML, like Gecko) Chrome/96.0.4664.110 "
      "Safari/537.36\r\n"
      "Accept-Language: zh-CN,zh;q=0.9,en-US;q=0.8,en;q=0.7\r\n"
      "Cookie: 123=456;\r\n\r\n";

  CHECK(parser.Parse(r2, strlen(r2)));
  auto request2 = parser.TakeRequest();
  CHECK(request2.GetMethod() == "GET");
  CHECK(request2.GetUrl() == "/someaddr?key1=value1#frag");
  CHECK(request2.GetVersion() == "HTTP/1.1");
  CHECK(request2.GetAllHeader().size() == 7);
  CHECK(request2.GetHeader("Host") == "www.xxxx.com:80");
  CHECK(request2.GetHeader("Connection") == "close");
  CHECK(request2.GetHeader("Accept") ==
        "application/json, text/javascript, */*; q=0.01");
  CHECK(request2.GetHeader("X-Requested-With") == "XMLHttpRequest");
  CHECK(request2.GetHeader("User-Agent") ==
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
        "(KHTML, like Gecko) Chrome/96.0.4664.110 Safari/537.36");
  CHECK(request2.GetHeader("Accept-Language") ==
        "zh-CN,zh;q=0.9,en-US;q=0.8,en;q=0.7");
  CHECK(request2.GetHeader("Cookie") == "123=456;");
}

TEST_CASE("Post with body") {
  Cold::Net::Http::HttpRequestParser parser;
  const char* r =
      "POST /addr HTTP/1.0\r\nConnection: close\r\nContent-Length: 35"
      "\r\n\r\nkey1=value1&";
  const char* r1 = "key2=value2&key3=value3";
  CHECK(parser.Parse(r, strlen(r)));
  CHECK(parser.Parse(r1, strlen(r1)));
  CHECK(parser.HasRequest());
  CHECK(parser.RequestQueueSize() == 1);
  auto request1 = parser.TakeRequest();
  CHECK(request1.GetMethod() == "POST");
  CHECK(request1.GetUrl() == "/addr");
  CHECK(request1.GetVersion() == "HTTP/1.0");
  {
    auto headers = request1.GetAllHeader();
    CHECK(headers.size() == 2);

    CHECK(headers.find("Connection") != headers.end());
    CHECK(headers.find("Connection")->second == "close");

    CHECK(headers.find("Content-Length") != headers.end());
    CHECK(headers.find("Content-Length")->second == "35");
  }
  CHECK(request1.GetBody().size() == 35);
  CHECK(request1.GetBody() == "key1=value1&key2=value2&key3=value3");
}

TEST_CASE("parse in multiple pieces") {
  {
    const char* r1 = "G";
    const char* r2 = "E";
    const char* r3 = "T";
    const char* r4 = " /addr?";
    const char* r5 = "key1=value1&";
    const char* r6 = "key1=value1&key2=value2";
    const char* r7 = " HT";
    const char* r8 = "TP/1.1\r\n\r\n";
    Cold::Net::Http::HttpRequestParser parser;
    CHECK(parser.Parse(r1, strlen(r1)));
    CHECK(parser.Parse(r2, strlen(r2)));
    CHECK(parser.Parse(r3, strlen(r3)));
    CHECK(parser.Parse(r4, strlen(r4)));
    CHECK(parser.Parse(r5, strlen(r5)));
    CHECK(parser.Parse(r6, strlen(r6)));
    CHECK(parser.Parse(r7, strlen(r7)));
    CHECK(parser.Parse(r8, strlen(r8)));
    CHECK(parser.HasRequest() == true);
    CHECK(parser.RequestQueueSize() == 1);
    auto p = parser.TakeRequest();
    CHECK(p.GetUrl() == "/addr?key1=value1&key1=value1&key2=value2");
  }
  const char* r1 =
      "GET / HTTP/1.1\r\n\r\nGET /someaddr?key1=value1#frag HTTP/1.1\r\n"
      "Host: www.xx";
  const char* r2 =
      "xx.com:80\r\n"
      "Connection: keep-alive\r\n"
      "Accept: application/";
  const char* r3 =
      "json, text/javascript, */*; q=0.01\r\n"
      "X-Requested-With: XMLHttpRequest\r\n"
      "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
      "AppleWebKit/537.36 ";
  const char* r4 =
      "(KHTML, like Gecko) Chrome/96.0.4664.110 "
      "Safari/537.36\r\n"
      "Accept-Language: zh-CN,zh;q=0.9,en-US;q=0.8,en;q=0.7\r\n"
      "Cookie: 123=456;\r\n\r\nPOST ";
  const char* r5 =
      "/addr HTTP/1.0\r\nConnection: "
      "keep-alive\r\nContent-Length: "
      "35\r\n\r\nkey1=value1&key2=value2&key3=value3";
  Cold::Net::Http::HttpRequestParser parser;
  CHECK(parser.Parse(r1, strlen(r1)));
  CHECK(parser.HasRequest() == true);
  CHECK(parser.RequestQueueSize() == 1);

  auto request1 = parser.TakeRequest();
  CHECK(request1.GetMethod() == "GET");
  CHECK(request1.GetUrl() == "/");
  CHECK(request1.GetVersion() == "HTTP/1.1");
  CHECK(request1.GetAllHeader().empty());
  CHECK(request1.GetBody().empty());

  CHECK(parser.Parse(r2, strlen(r2)));
  CHECK(parser.HasRequest() == false);
  CHECK(parser.RequestQueueSize() == 0);
  CHECK(parser.Parse(r3, strlen(r3)));
  CHECK(parser.HasRequest() == false);
  CHECK(parser.RequestQueueSize() == 0);
  CHECK(parser.Parse(r4, strlen(r4)));
  CHECK(parser.HasRequest() == true);
  CHECK(parser.RequestQueueSize() == 1);

  auto request2 = parser.TakeRequest();
  CHECK(request2.GetMethod() == "GET");
  CHECK(request2.GetUrl() == "/someaddr?key1=value1#frag");
  CHECK(request2.GetVersion() == "HTTP/1.1");
  {
    auto headers = request2.GetAllHeader();
    CHECK(headers.size() == 7);

    CHECK(headers.find("Host") != headers.end());
    CHECK(headers.find("Host")->second == "www.xxxx.com:80");

    CHECK(headers.find("Connection") != headers.end());
    CHECK(headers.find("Connection")->second == "keep-alive");

    CHECK(headers.find("Accept") != headers.end());
    CHECK(headers.find("Accept")->second ==
          "application/json, text/javascript, */*; q=0.01");

    CHECK(headers.find("X-Requested-With") != headers.end());
    CHECK(headers.find("X-Requested-With")->second == "XMLHttpRequest");

    CHECK(headers.find("User-Agent") != headers.end());
    CHECK(headers.find("User-Agent")->second ==
          "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
          "(KHTML, like Gecko) Chrome/96.0.4664.110 Safari/537.36");

    CHECK(headers.find("Accept-Language") != headers.end());
    CHECK(headers.find("Accept-Language")->second ==
          "zh-CN,zh;q=0.9,en-US;q=0.8,en;q=0.7");

    CHECK(headers.find("Cookie") != headers.end());
    CHECK(headers.find("Cookie")->second == "123=456;");
  }
  CHECK(request2.GetBody().empty());

  CHECK(parser.Parse(r5, strlen(r5)));
  CHECK(parser.HasRequest() == true);
  CHECK(parser.RequestQueueSize() == 1);

  auto request3 = parser.TakeRequest();

  CHECK(request3.GetMethod() == "POST");
  CHECK(request3.GetUrl() == "/addr");
  CHECK(request3.GetVersion() == "HTTP/1.0");
  {
    auto headers = request3.GetAllHeader();
    CHECK(headers.size() == 2);

    CHECK(headers.find("Connection") != headers.end());
    CHECK(headers.find("Connection")->second == "keep-alive");

    CHECK(headers.find("Content-Length") != headers.end());
    CHECK(headers.find("Content-Length")->second == "35");
  }
  CHECK(request3.GetBody().size() == 35);
  CHECK(request3.GetBody() == "key1=value1&key2=value2&key3=value3");
}

TEST_CASE("bad request") {
  const char* r1 = "OPTIONSx";
  {
    Cold::Net::Http::HttpRequestParser parser;
    CHECK(!parser.Parse(r1, strlen(r1)));
  }
  {
    const char* r2 =
        "POST / HTTP/1.0\r\nConnection: close\r\nContent-Length: "
        "xxx\r\n\r\nxx ";
    Cold::Net::Http::HttpRequestParser parser;
    CHECK(!parser.Parse(r2, strlen(r2)));
  }
  {
    const char* r3 = "XXX / HTTP/1.0\r\n\r\n";
    Cold::Net::Http::HttpRequestParser parser;
    CHECK(!parser.Parse(r3, strlen(r3)));
  }

  {
    const char* r4 = "XXX / HTTP/1.0\r\n\r\n";
    Cold::Net::Http::HttpRequestParser parser;
    CHECK(!parser.Parse(r4, strlen(r4)));
  }

  {
    const char* r5 = "GET / HTTP/3.3\r\n\r\n";
    Cold::Net::Http::HttpRequestParser parser;
    CHECK(!parser.Parse(r5, strlen(r5)));
  }
}

TEST_CASE("test request too large") {
  size_t maxUrlSize = Cold::Base::Config::GetGloablDefaultConfig().GetOrDefault(
      "/http/max-url-size", 190000ull);  // 185K
  size_t maxHeaderFieldSize =
      Cold::Base::Config::GetGloablDefaultConfig().GetOrDefault(
          "/http/max-header-field-size", 1024ull);  // 1K
  size_t maxHeaderValueSize =
      Cold::Base::Config::GetGloablDefaultConfig().GetOrDefault(
          "/http/max-header-value-size", 10 * 1024ull);  // 10K
  size_t maxBodySize =
      Cold::Base::Config::GetGloablDefaultConfig().GetOrDefault(
          "/http/max-body-size", 1024 * 1024ull);  // 1M
  size_t maxHeadersCount =
      Cold::Base::Config::GetGloablDefaultConfig().GetOrDefault(
          "/http/max-headers-count", 100ull);  // 30M
  {
    std::string url(maxUrlSize, 'a');
    Cold::Net::Http::HttpRequestParser parser;
    auto req = fmt::format("GET /{} HTTP/1.1", url);
    CHECK(!parser.Parse(req.data(), req.size()));
    CHECK(std::string_view(parser.GetBadReason()) == "Url size is too large");
  }
  {
    std::string field(maxHeaderFieldSize + 1, 'a');
    Cold::Net::Http::HttpRequestParser parser;
    auto req = fmt::format("GET / HTTP/1.1\r\n{}: 1\r\n\r\n", field);
    CHECK(!parser.Parse(req.data(), req.size()));
    CHECK(std::string_view(parser.GetBadReason()) ==
          "Header field size is too large");
  }
  {
    std::string value(maxHeaderValueSize + 1, 'a');
    Cold::Net::Http::HttpRequestParser parser;
    auto req = fmt::format("GET / HTTP/1.1\r\nfield: {}\r\n\r\n", value);
    CHECK(!parser.Parse(req.data(), req.size()));
    CHECK(std::string_view(parser.GetBadReason()) ==
          "Header value size is too large");
  }
  {
    std::string body(maxBodySize + 1, 'a');
    Cold::Net::Http::HttpRequestParser parser;
    auto req = fmt::format("GET / HTTP/1.1\r\nContent-Length: {}\r\n\r\n{}",
                           body.size(), body);
    CHECK(!parser.Parse(req.data(), req.size()));
    CHECK(std::string_view(parser.GetBadReason()) == "Body size is too large");
  }
  {
    std::string headers;
    for (size_t i = 0; i < maxHeadersCount + 1; ++i) {
      headers += std::to_string(i);
      headers += ": ";
      headers += std::to_string(i);
      headers += "\r\n";
    }
    Cold::Net::Http::HttpRequestParser parser;
    auto req = fmt::format("GET / HTTP/1.1\r\n{}\r\n", headers);
    CHECK(!parser.Parse(req.data(), req.size()));
    CHECK(std::string_view(parser.GetBadReason()) == "Headers is too large");
  }
}

TEST_CASE("test") {
  const char* req =
      "GET "
      "/index.html%3f%e5%8f%82%e6%95%b0%e4%b8%80%3d%e5%80%bc+%e4%b8%80%26%e5%"
      "8f%82%e6%95%b0%e4%ba%8c%3d%e5%80%bc%e4%ba%8c%26%e5%8f%82%e6%95%b0%e4%b8%"
      "89%3d%e5%80%bc%e4%b8%89%26key4%3dvalue4 HTTP/1.0\r\n\r\n";
  // const char* s = "?参数一=值 一&参数二=值二&参数三=值三&key4=value4";
  Cold::Net::Http::HttpRequestParser parser;
  CHECK(parser.Parse(req, strlen(req)));
  CHECK(parser.HasRequest());
  auto request = parser.TakeRequest();
  CHECK(request.GetUrl() ==
        "/index.html%3f%e5%8f%82%e6%95%b0%e4%b8%80%3d%e5%80%bc+%e4%b8%80%26%e5%"
        "8f%82%e6%95%b0%e4%ba%8c%3d%e5%80%bc%e4%ba%8c%26%e5%8f%82%e6%95%b0%e4%"
        "b8%89%3d%e5%80%bc%e4%b8%89%26key4%3dvalue4");
}

TEST_CASE("test websocket") {
  const char* str =
      "GET / HTTP/1.1\r\n"
      "Host: localhost:8080\r\n"
      "Origin: http://127.0.0.1:3000\r\n"
      "Connection: Upgrade\r\n"
      "Upgrade: websocket\r\n"
      "Sec-WebSocket-Version: 13\r\n"
      "Sec-WebSocket-Key: w4v7O6xFTi36lq3RNcgctw==\r\n\r\n";
  Cold::Net::Http::HttpRequestParser parser;
  CHECK(parser.Parse(str, strlen(str)));
  CHECK(parser.HasRequest());
  auto request = parser.TakeRequest();
  CHECK(request.GetMethod() == "GET");
  CHECK(request.GetUrl() == "/");
  CHECK(request.GetVersion() == "HTTP/1.1");
  CHECK(request.GetHeader("Host") == "localhost:8080");
  CHECK(request.GetHeader("Origin") == "http://127.0.0.1:3000");
  CHECK(request.GetHeader("Connection") == "Upgrade");
  CHECK(request.GetHeader("Upgrade") == "websocket");
  CHECK(request.GetHeader("Sec-WebSocket-Version") == "13");
  CHECK(request.GetHeader("Sec-WebSocket-Key") == "w4v7O6xFTi36lq3RNcgctw==");
}