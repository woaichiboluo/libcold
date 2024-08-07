# time
add_executable(TimeTest time/TimeTest.cpp)
target_link_libraries(TimeTest PRIVATE cold)
set_target_properties(TimeTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/time")
add_test(NAME TimeTest COMMAND TimeTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/time)

add_executable(TimerTest time/TimerTest.cpp)
target_link_libraries(TimerTest PRIVATE cold)
set_target_properties(TimerTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/time")

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

# log 
add_executable(LogFormatterTest log/LogFormatterTest.cpp)
target_link_libraries(LogFormatterTest PRIVATE cold)
set_target_properties(LogFormatterTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/log")
add_test(NAME LogFormatterTest COMMAND LogFormatterTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/log)

add_executable(LoggerTest log/LoggerTest.cpp)
target_link_libraries(LoggerTest PRIVATE cold)
set_target_properties(LoggerTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/log")

add_executable(LogSinkTest log/LogSinkTest.cpp)
target_link_libraries(LogSinkTest PRIVATE cold)
set_target_properties(LogSinkTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/log")

add_executable(ConditionTest thread/ConditionTest.cpp)
target_link_libraries(ConditionTest PRIVATE cold)
set_target_properties(ConditionTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/thread")

# coro
add_executable(TaskTest coro/TaskTest.cpp)
target_link_libraries(TaskTest PRIVATE cold)
set_target_properties(TaskTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/coro")
add_test(NAME TaskTest COMMAND TaskTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/log)

add_executable(AsyncMutexTest coro/AsyncMutexTest.cpp)
target_link_libraries(AsyncMutexTest PRIVATE cold)
set_target_properties(AsyncMutexTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/coro")

add_executable(ConditionVariableTest coro/ConditionVariableTest.cpp)
target_link_libraries(ConditionVariableTest PRIVATE cold)
set_target_properties(ConditionVariableTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/coro")

# net
add_executable(IpAddressTest net/IpAddressTest.cpp)
target_link_libraries(IpAddressTest PRIVATE cold)
set_target_properties(IpAddressTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/net")
add_test(NAME IpAddressTest COMMAND IpAddressTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/net)

add_executable(SocketOptionsTest net/SocketOptionsTest.cpp)
target_link_libraries(SocketOptionsTest PRIVATE cold)
set_target_properties(SocketOptionsTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/net")
add_test(NAME SocketOptionsTest COMMAND SocketOptionsTest  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/net)

add_executable(AcceptorTest net/AcceptorTest.cpp)
target_link_libraries(AcceptorTest PRIVATE cold)
set_target_properties(AcceptorTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/net")

add_executable(BasicEchoServerTest net/BasicEchoServerTest.cpp)
target_link_libraries(BasicEchoServerTest PRIVATE cold)
set_target_properties(BasicEchoServerTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/net")

add_executable(BasicUdpEchoServerTest net/BasicUdpEchoServerTest.cpp)
target_link_libraries(BasicUdpEchoServerTest PRIVATE cold)
set_target_properties(BasicUdpEchoServerTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/net")

add_executable(ConnectWithTimeoutTest net/ConnectWithTimeoutTest.cpp)
target_link_libraries(ConnectWithTimeoutTest PRIVATE cold)
set_target_properties(ConnectWithTimeoutTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/net")

add_executable(IoTimeoutTest net/IoTimeoutTest.cpp)
target_link_libraries(IoTimeoutTest PRIVATE cold)
set_target_properties(IoTimeoutTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/net")

add_executable(HttpRequestParserTest net/http/HttpRequestParserTest.cpp)
target_link_libraries(HttpRequestParserTest PRIVATE cold)
set_target_properties(HttpRequestParserTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/net")
add_test(NAME HttpRequestParserTest COMMAND HttpRequestParserTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/net)

add_executable(RouterTest net/http/RouterTest.cpp)
target_link_libraries(RouterTest PRIVATE cold)
set_target_properties(RouterTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/net")
add_test(NAME RouterTest COMMAND RouterTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/net)

#util
add_executable(ConfigTest util/ConfigTest.cpp)
target_link_libraries(ConfigTest PRIVATE cold)
add_test(NAME ConfigTest COMMAND ConfigTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests/util)
set_target_properties(ConfigTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests/util")