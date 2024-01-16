set(SRC time/Time.cpp util/StringUtil.cpp thread/Thread.cpp log/LogFormatter.cpp)

add_executable(TimeTest time/test/TimeTest.cpp ${SRC})
set_target_properties(TimeTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/time")

add_executable(StringUtilTest util/test/StringUtilTest.cpp ${SRC})
set_target_properties(StringUtilTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/util")

add_executable(LockTest thread/test/LockTest.cpp ${SRC})
set_target_properties(LockTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/thread")

add_executable(ConditionTest thread/test/ConditionTest.cpp ${SRC})
set_target_properties(ConditionTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/thread")

add_executable(ThreadTest thread/test/ThreadTest.cpp ${SRC})
set_target_properties(ThreadTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/thread")

add_executable(LogFormatterTest log/test/LogFormatterTest.cpp ${SRC})
set_target_properties(LogFormatterTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/log")

add_test(NAME TimeTest COMMAND TimeTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/time)
add_test(NAME StringUtilTest COMMAND StringUtilTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/util)
add_test(NAME ThreadTest COMMAND ThreadTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/thread)
add_test(NAME LogFormatterTest COMMAND LogFormatterTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/log)