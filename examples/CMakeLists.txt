include_directories(${CMAKE_SOURCE_DIR})

add_executable(ShellColorShow base/ShellColorShow.cpp)


# net chatroom
add_executable(ChatServer net/chatroom/ChatServer.cpp)
add_executable(ChatClient net/chatroom/ChatClient.cpp)

# net ttcp
add_executable(TtcpSender net/ttcp/TtcpSender.cpp)
add_executable(TtcpReceiver net/ttcp/TtcpReceiver.cpp)

# net simple
add_subdirectory(net/simple)

# net filetransfer
add_executable(FileTransfer net/filetransfer/Transfer.cpp)

# net socks4a
add_executable(TcpRelay net/socks4a/TcpRelay.cpp)
add_executable(Socks4a net/socks4a/Socks4a.cpp)

if(${COLD_SSL_AVAILABLE})
    add_subdirectory(net/ssl)
endif()

# net simplehttp
add_subdirectory(net/simplehttp)

if(${COLD_HTTP_AVAILABLE})
    add_subdirectory(net/http)
endif()