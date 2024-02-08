set(
    COLD_SRC time/Time.cpp
    util/StringUtil.cpp
    thread/Thread.cpp
    log/LogFormatter.cpp log/Logger.cpp
    coro/IoWatcherEpoll.cpp coro/IoContext.cpp coro/Timer.cpp coro/TimeQueue.cpp coro/IoContextPool.cpp
    net/IpAddress.cpp net/BasicSocket.cpp net/Acceptor.cpp
)

add_library(cold ${COLD_SRC})

# time
add_executable(TimeTest time/test/TimeTest.cpp)
target_link_libraries(TimeTest PRIVATE cold)
set_target_properties(TimeTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/time")

# util
add_executable(StringUtilTest util/test/StringUtilTest.cpp)
target_link_libraries(StringUtilTest PRIVATE cold)
set_target_properties(StringUtilTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/util")

# thread
add_executable(LockTest thread/test/LockTest.cpp)
target_link_libraries(LockTest PRIVATE cold)
set_target_properties(LockTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/thread")

add_executable(ConditionTest thread/test/ConditionTest.cpp)
target_link_libraries(ConditionTest PRIVATE cold)
set_target_properties(ConditionTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/thread")

add_executable(ThreadTest thread/test/ThreadTest.cpp)
target_link_libraries(ThreadTest PRIVATE cold)
set_target_properties(ThreadTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/thread")

# log
add_executable(LogFormatterTest log/test/LogFormatterTest.cpp)
target_link_libraries(LogFormatterTest PRIVATE cold)
set_target_properties(LogFormatterTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/log")

add_executable(LoggerTest log/test/LoggerTest.cpp)
target_link_libraries(LoggerTest PRIVATE cold)
set_target_properties(LoggerTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/log")

add_executable(LogSinkTest log/test/LogSinkTest.cpp)
target_link_libraries(LogSinkTest PRIVATE cold)
set_target_properties(LogSinkTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/log")

# coro
add_executable(TaskTest coro/test/TaskTest.cpp)
target_link_libraries(TaskTest PRIVATE cold)
set_target_properties(TaskTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/coro")

add_executable(TimerTest coro/test/TimerTest.cpp)
target_link_libraries(TimerTest PRIVATE cold)
set_target_properties(TimerTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/coro")

add_executable(IpAddressTest net/test/IpAddressTest.cpp)
target_link_libraries(IpAddressTest PRIVATE cold)
set_target_properties(IpAddressTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/net")

add_executable(SocketOptionsTest net/test/SocketOptionsTest.cpp)
target_link_libraries(SocketOptionsTest PRIVATE cold)
set_target_properties(SocketOptionsTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/net")

add_executable(AcceptorTest net/test/AcceptorTest.cpp)
target_link_libraries(AcceptorTest PRIVATE cold)
set_target_properties(AcceptorTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/net")

add_executable(BasicEchoServerTest net/test/BasicEchoServerTest.cpp)
target_link_libraries(BasicEchoServerTest PRIVATE cold)
set_target_properties(BasicEchoServerTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/net")

# time
add_test(NAME TimeTest COMMAND TimeTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/time)

# util
add_test(NAME StringUtilTest COMMAND StringUtilTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/util)

# thread
add_test(NAME LockTest COMMAND LockTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/thread)
add_test(NAME ConditionTest COMMAND ConditionTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/thread)
add_test(NAME ThreadTest COMMAND ThreadTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/thread)

# log
add_test(NAME LogFormatterTest COMMAND LogFormatterTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/log)
add_test(NAME LoggerTest COMMAND LoggerTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/log)
add_test(NAME LogSinkTest COMMAND LogSinkTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/log)

# coro
add_test(NAME TaskTest COMMAND TaskTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/coro)
add_test(NAME TimerTest COMMAND TimerTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/coro)

# net
add_test(NAME IpAddressTest COMMAND IpAddressTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/net)
add_test(NAME SocketOptionsTest COMMAND SocketOptionsTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/net)