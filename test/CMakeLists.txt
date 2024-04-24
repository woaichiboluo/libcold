add_executable(TimeTest time/TimeTest.cpp)
target_link_libraries(TimeTest PRIVATE cold)
set_target_properties(TimeTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/time")