# simple/simplehttp
set(simplehttpsource
  simple/simplehttp/RequestParser.cpp
  simple/simplehttp/HttpResponse.cpp
)

add_executable(RequestParserTest ${simplehttpsource} simple/simplehttp/RequestParserTest.cpp)
target_link_libraries(RequestParserTest PRIVATE cold)

add_executable(SimpleHttpServer simple/simplehttp/HttpServer.cpp ${simplehttpsource})
target_link_libraries(SimpleHttpServer PRIVATE cold)

# asio/chatroom
add_executable(AsioChatRoomServer asio/chatroom/ChatServer.cpp)
target_link_libraries(AsioChatRoomServer PRIVATE cold)

add_executable(AsioChatRoomClient asio/chatroom/ChatClient.cpp)
target_link_libraries(AsioChatRoomClient PRIVATE cold)

add_executable(ShellColorShow util/ShellColorShow.cpp)
target_link_libraries(ShellColorShow PRIVATE cold)

# echo
add_executable(EchoServer simple/echo/EchoServer.cpp)
target_link_libraries(EchoServer PRIVATE cold)

add_executable(EchoClient simple/echo/EchoClient.cpp)
target_link_libraries(EchoClient PRIVATE cold)

# http
add_executable(HttpExample1 http/example1.cpp)
target_link_libraries(HttpExample1 PRIVATE cold)

add_executable(HttpExample2 http/example2.cpp)
target_link_libraries(HttpExample2 PRIVATE cold)

add_executable(HttpExample3 http/example3.cpp)
target_link_libraries(HttpExample3 PRIVATE cold)

if(${BUILD_SSL_MODE})
  add_executable(HttpExample4 http/example4.cpp)
  target_link_libraries(HttpExample4 PRIVATE cold)
endif()

if(${BUILD_PROTOBUF_MODE})
  add_subdirectory(rpc)
endif()