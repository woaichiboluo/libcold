# simple/simplehttp
set(simplehttpsource 
  simple/simplehttp/RequestParser.cpp
  simple/simplehttp/HttpResponse.cpp
  )

add_executable(RequestParserTest ${simplehttpsource} simple/simplehttp/RequestParserTest.cpp)
target_link_libraries(RequestParserTest cold)

add_executable(SimpleHttpServer  simple/simplehttp/HttpServer.cpp ${simplehttpsource})
target_link_libraries(SimpleHttpServer cold)
