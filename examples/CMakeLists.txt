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

# http
add_executable(HttpExample1 http/example1.cpp)
target_link_libraries(HttpExample1 PRIVATE cold)

add_executable(HttpExample2 http/example2.cpp)
target_link_libraries(HttpExample2 PRIVATE cold)