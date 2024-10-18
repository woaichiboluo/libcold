#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "HttpRequest.h"
#include "RequestParser.h"
#include "third_party/doctest.h"

TEST_CASE("basic") {
  const char* r1 = "GET / HTTP/1.1\r\n\r\n";
  RequestParser parser;
  HttpRequest request1;
  CHECK(parser.Parse(r1, request1) == RequestParser::kOK);
  CHECK(request1.GetMethod() == "GET");
  CHECK(request1.GetUri() == "/");
  CHECK(request1.GetQuery().empty());
  CHECK(request1.GetFragment().empty());
  CHECK(request1.GetVersion() == "HTTP/1.1");
  CHECK(request1.GetHeaders().empty());
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

  HttpRequest request2;
  CHECK(parser.Parse(r2, request2) == RequestParser::kOK);
  CHECK(request2.GetMethod() == "GET");
  CHECK(request2.GetUri() == "/someaddr");
  CHECK(request2.GetQuery() == "key1=value1");
  CHECK(request2.GetFragment() == "frag");
  CHECK(request2.GetVersion() == "HTTP/1.1");
  {
    auto headers = request2.GetHeaders();
    CHECK(headers.size() == 7);

    CHECK(headers.find("Host") != headers.end());
    CHECK(headers.find("Host")->second == "www.xxxx.com:80");

    CHECK(headers.find("Connection") != headers.end());
    CHECK(headers.find("Connection")->second == "close");

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
}

TEST_CASE("Post with body") {
  RequestParser parser;
  HttpRequest request1;
  const char* r =
      "POST /addr HTTP/1.0\r\nConnection: close\r\nContent-Length: 35"
      "\r\n\r\nkey1=value1&";
  const char* r1 = "key2=value2&key3=value3";
  CHECK(parser.Parse(r, request1) == RequestParser::kKEEP);
  CHECK(parser.Parse(r1, request1) == RequestParser::kOK);
  CHECK(request1.GetMethod() == "POST");
  CHECK(request1.GetUri() == "/addr");
  CHECK(request1.GetQuery().empty());
  CHECK(request1.GetFragment().empty());
  CHECK(request1.GetVersion() == "HTTP/1.0");
  {
    auto headers = request1.GetHeaders();
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
  const char* r1 =
      "GET / HTTP/1.1\r\n\r\nGET /someaddr?key1=value1#frag HTTP/1.1\r\n"
      "Host: www.xx";
  const char* r2 =
      "xx.com:80\r\n"
      "Connection: close\r\n"
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
      "close\r\nContent-Length: 35\r\n\r\nkey1=value1&key2=value2&key3=value3";
  HttpRequest request1, request2, request3;
  RequestParser parser;
  CHECK(parser.Parse(r1, request1) == RequestParser::kOK);
  CHECK(request1.GetMethod() == "GET");
  CHECK(request1.GetUri() == "/");
  CHECK(request1.GetQuery().empty());
  CHECK(request1.GetFragment().empty());
  CHECK(request1.GetVersion() == "HTTP/1.1");
  CHECK(request1.GetHeaders().empty());
  CHECK(request1.GetBody().empty());
  CHECK(parser.Parse(r2, request2) == RequestParser::kKEEP);
  CHECK(parser.Parse(r3, request2) == RequestParser::kKEEP);
  CHECK(parser.Parse(r4, request2) == RequestParser::kOK);
  CHECK(request2.GetMethod() == "GET");
  CHECK(request2.GetUri() == "/someaddr");
  CHECK(request2.GetQuery() == "key1=value1");
  CHECK(request2.GetFragment() == "frag");
  CHECK(request2.GetVersion() == "HTTP/1.1");
  {
    auto headers = request2.GetHeaders();
    CHECK(headers.size() == 7);

    CHECK(headers.find("Host") != headers.end());
    CHECK(headers.find("Host")->second == "www.xxxx.com:80");

    CHECK(headers.find("Connection") != headers.end());
    CHECK(headers.find("Connection")->second == "close");

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
  CHECK(parser.Parse(r5, request3) == RequestParser::kOK);
  CHECK(request3.GetMethod() == "POST");
  CHECK(request3.GetUri() == "/addr");
  CHECK(request3.GetQuery().empty());
  CHECK(request3.GetFragment().empty());
  CHECK(request3.GetVersion() == "HTTP/1.0");
  {
    auto headers = request3.GetHeaders();
    CHECK(headers.size() == 2);

    CHECK(headers.find("Connection") != headers.end());
    CHECK(headers.find("Connection")->second == "close");

    CHECK(headers.find("Content-Length") != headers.end());
    CHECK(headers.find("Content-Length")->second == "35");
  }
  CHECK(request3.GetBody().size() == 35);
  CHECK(request3.GetBody() == "key1=value1&key2=value2&key3=value3");
}

TEST_CASE("bad request") {
  const char* r1 = "OPTIONSx";
  RequestParser parser;
  HttpRequest request1;
  CHECK(parser.Parse(r1, request1) == RequestParser::kBAD);
  parser.Reset();
  const char* r2 =
      "POST / HTTP/1.0\r\nConnection: close\r\nContent-Length: xxx\r\n\r\nxx";
  CHECK(parser.Parse(r2, request1) == RequestParser::kBAD);
  parser.Reset();
  const char* r3 = "XXX / HTTP/1.0\r\n\r\n";
  CHECK(parser.Parse(r3, request1) == RequestParser::kBAD);
  parser.Reset();
  const char* r4 = "POST /addr HTTP/1.0\r\n\r\nbad request";
  CHECK(parser.Parse(r4, request1) == RequestParser::kOK);
  CHECK(parser.Parse("", request1) == RequestParser::kBAD);
}