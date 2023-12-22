set(SRC time/Time.cpp util/StringUtil.cpp)

# time test
add_executable(TimeTest time/test/TimeTest.cpp ${SRC})
set_target_properties(TimeTest PROPERTIES  
                    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/time")
add_test(NAME TimeTest COMMAND TimeTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/time)

# StringUtil test
add_executable(StringUtilTest util/test/StringUtilTest.cpp ${SRC})
set_target_properties(TimeTest PROPERTIES  
                    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/util")
add_test(NAME StringUtilTest COMMAND StringUtilTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/util)