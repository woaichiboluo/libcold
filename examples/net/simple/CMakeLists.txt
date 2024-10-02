# set bin
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/examples)

# chargen
add_executable(ChargenServer chargen/ChargenServer.cpp)

# discard
add_executable(DiscardServer discard/Discard.cpp)

# echo
add_executable(EchoServer echo/EchoServer.cpp)
add_executable(EchoClient echo/EchoClient.cpp)

# daytime
add_executable(DaytimeServer daytime/DaytimeServer.cpp)

# time
add_executable(TimeServer time/TimeServer.cpp)
add_executable(TimeClient time/TimeClient.cpp)