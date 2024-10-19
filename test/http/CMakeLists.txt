
if(${COLD_HTTP_AVAILABLE})
    add_executable(HttpRequestParserTest HttpRequestParserTest.cpp)
    target_link_libraries(HttpRequestParserTest llhttp)
    add_test(NAME HttpRequestParserTest COMMAND HttpRequestParserTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

    add_executable(RouterTest RouterTest.cpp)
    target_link_libraries(RouterTest llhttp)
    add_test(NAME RouterTest COMMAND RouterTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
endif()