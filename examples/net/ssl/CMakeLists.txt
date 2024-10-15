set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/examples)


add_executable(EchoServerWithSSL EchoServer.cpp)
target_link_libraries(EchoServerWithSSL ${OPENSSL_LIBRARIES})

add_executable(EchoClientWithSSL EchoClient.cpp)
target_link_libraries(EchoClientWithSSL ${OPENSSL_LIBRARIES})