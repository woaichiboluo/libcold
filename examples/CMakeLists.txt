# simple/simplehttp
set(simplehttpsource
  simple/simplehttp/RequestParser.cpp
  simple/simplehttp/HttpResponse.cpp
)

# simple/echo
add_executable(EchoServer simple/echo/EchoServer.cpp)
target_link_libraries(EchoServer PRIVATE cold)

add_executable(EchoClient simple/echo/EchoClient.cpp)
target_link_libraries(EchoClient PRIVATE cold)

# simple/chargen
add_executable(ChargenServer simple/chargen/ChargenServer.cpp)
target_link_libraries(ChargenServer PRIVATE cold)

add_executable(RequestParserTest ${simplehttpsource} simple/simplehttp/RequestParserTest.cpp)
target_link_libraries(RequestParserTest PRIVATE cold)

add_executable(SimpleHttpServer simple/simplehttp/HttpServer.cpp ${simplehttpsource})
target_link_libraries(SimpleHttpServer PRIVATE cold)

# simple/ttcp 
add_executable(TtcpBlocking simple/ttcp/TtcpBlocking.cpp)
target_link_libraries(TtcpBlocking PRIVATE cold)

add_executable(TtcpNonBlocking simple/ttcp/TtcpNonBlocking.cpp)
target_link_libraries(TtcpNonBlocking PRIVATE cold)


# asio/chatroom
add_executable(AsioChatRoomServer asio/chatroom/ChatServer.cpp)
target_link_libraries(AsioChatRoomServer PRIVATE cold)

add_executable(AsioChatRoomClient asio/chatroom/ChatClient.cpp)
target_link_libraries(AsioChatRoomClient PRIVATE cold)

add_executable(ShellColorShow util/ShellColorShow.cpp)
target_link_libraries(ShellColorShow PRIVATE cold)


# http
add_executable(HttpExample1 http/example1.cpp)
target_link_libraries(HttpExample1 PRIVATE cold)

add_executable(HttpExample2 http/example2.cpp)
target_link_libraries(HttpExample2 PRIVATE cold)

add_executable(HttpExample3 http/example3.cpp)
target_link_libraries(HttpExample3 PRIVATE cold)

add_executable(HttpExample4 http/example4.cpp)
target_link_libraries(HttpExample4 PRIVATE cold)


# ssl
if(${BUILD_SSL_MODE})
# https
  add_executable(HttpsServer ssl/HttpsServer.cpp)
  target_link_libraries(HttpsServer PRIVATE cold)
  # daytime
  add_executable(DaytimeServer ssl/daytime/DaytimeServer.cpp)
  target_link_libraries(DaytimeServer PRIVATE cold)

  add_executable(DaytimeClient ssl/daytime/DaytimeClient.cpp)
  target_link_libraries(DaytimeClient PRIVATE cold)
endif()

if(${BUILD_PROTOBUF_MODE})
  add_subdirectory(rpc)
endif()