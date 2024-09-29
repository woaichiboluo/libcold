# set bin
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/examples)

# chargen
add_executable(ChargenServer chargen/ChargenServer.cpp)

# ttcp
add_executable(TtcpSender ttcp/TtcpSender.cpp)
add_executable(TtcpReceiver ttcp/TtcpReceiver.cpp)