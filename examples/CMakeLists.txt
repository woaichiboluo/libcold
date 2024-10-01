include_directories(${CMAKE_SOURCE_DIR})

add_executable(ShellColorShow base/ShellColorShow.cpp)

add_subdirectory(net/simple)

add_executable(ChatServer net/asio/chatroom/ChatServer.cpp)
add_executable(ChatClient net/asio/chatroom/ChatClient.cpp)