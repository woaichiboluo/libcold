set(SRC time/Time.cpp)

# time test
add_executable(TimeTest time/test/TimeTest.cpp ${SRC})
set_target_properties(TimeTest PROPERTIES  
                    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/time")
add_test(NAME TimeTest COMMAND TimeTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/time)