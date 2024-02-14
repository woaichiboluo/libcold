add_executable(TcpEchoServer simple/echo/TcpEchoServer.cpp)
target_link_libraries(TcpEchoServer PRIVATE cold)

add_executable(TcpEchoClient simple/echo/TcpEchoClient.cpp)
target_link_libraries(TcpEchoClient PRIVATE cold)

add_executable(UdpEchoServer simple/echo/UdpEchoServer.cpp)
target_link_libraries(UdpEchoServer PRIVATE cold)

add_executable(UdpEchoClient simple/echo/UdpEchoClient.cpp)
target_link_libraries(UdpEchoClient PRIVATE cold)

add_executable(TcpAndUdpBothEcho simple/echo/TcpAndUdpBothEcho.cpp)
target_link_libraries(TcpAndUdpBothEcho PRIVATE cold)

add_executable(TcpDaytimeServer simple/daytime/TcpDaytimeServer.cpp)
target_link_libraries(TcpDaytimeServer PRIVATE cold)

add_executable(TcpDaytimeClient simple/daytime/TcpDaytimeClient.cpp)
target_link_libraries(TcpDaytimeClient PRIVATE cold)

add_executable(UdpDaytimeServer simple/daytime/UdpDaytimeServer.cpp)
target_link_libraries(UdpDaytimeServer PRIVATE cold)

add_executable(UdpDaytimeClient simple/daytime/UdpDaytimeClient.cpp)
target_link_libraries(UdpDaytimeClient PRIVATE cold)

add_executable(TcpAndUdpBothDaytime simple/daytime/TcpAndUdpBothDaytime.cpp)
target_link_libraries(TcpAndUdpBothDaytime PRIVATE cold)

add_executable(ConnectTimeout timeouts/ConnectTimeout.cpp)
target_link_libraries(ConnectTimeout PRIVATE cold)