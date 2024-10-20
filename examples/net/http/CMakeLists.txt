set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/examples)

add_executable(BasicHttpServer BasicHttpServer.cpp)
target_link_libraries(BasicHttpServer llhttp uuid)

add_executable(HttpLogin HttpLogin.cpp)
target_link_libraries(HttpLogin llhttp uuid)


if(${COLD_SSL_AVAILABLE})
    add_executable(BasicHttpsServer BasicHttpsServer.cpp)
    target_link_libraries(BasicHttpsServer llhttp uuid)
    target_link_libraries(BasicHttpsServer ${OPENSSL_LIBRARIES})
endif()

add_executable(HttpServerWithStaticFile HttpServerWithStaticFile.cpp)
target_link_libraries(HttpServerWithStaticFile llhttp uuid)

add_executable(HttpFileService HttpFileService.cpp)
target_link_libraries(HttpFileService llhttp uuid)