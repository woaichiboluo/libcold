set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/examples)

if(${BUILD_TEST})
    add_executable(SimpleHttpRequestParserTest RequestParserTest.cpp RequestParser.cpp)
    set_target_properties(SimpleHttpRequestParserTest PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test")
    add_test(NAME SimpleHttpRequestParserTest COMMAND SimpleHttpRequestParserTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/test)
endif()

set(SIMPLE_HTTP_SOURCES HttpResponse.cpp RequestParser.cpp)

if(${COLD_SSL_AVAILABLE})
    add_executable(SimpleHttpsServer HttpsServer.cpp ${SIMPLE_HTTP_SOURCES})
    target_link_libraries(SimpleHttpsServer ${OPENSSL_LIBRARIES})
endif()

add_executable(SimpleHttpServer HttpServer.cpp ${SIMPLE_HTTP_SOURCES})