# time
add_executable(TimeTest time/TimeTest.cpp)
target_link_libraries(TimeTest PRIVATE cold)
set_target_properties(TimeTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/time")
add_test(NAME TimeTest COMMAND TimeTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/time)

# thread
add_executable(LockTest thread/LockTest.cpp)
target_link_libraries(LockTest PRIVATE cold)
set_target_properties(LockTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/thread")

add_executable(ThreadTest thread/ThreadTest.cpp)
target_link_libraries(ThreadTest PRIVATE cold)
set_target_properties(ThreadTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/thread")
add_test(NAME ThreadTest COMMAND ThreadTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/thread)

add_executable(ConditionTest thread/ConditionTest.cpp)
target_link_libraries(ConditionTest PRIVATE cold)
set_target_properties(ConditionTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/thread")
