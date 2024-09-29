include_directories(${CMAKE_SOURCE_DIR})

# time
add_executable(TimeTest time/TimeTest.cpp)
add_test(NAME TimeTest COMMAND TimeTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable(TimerTest time/TimerTest.cpp)

add_executable(TimeoutAndSleepTest time/TimeoutAndSleepTest.cpp)

# log
add_executable(LogFormatterTest log/LogFormatterTest.cpp)
add_test(NAME LogFormatterTest COMMAND LogFormatterTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable(LoggerTest log/LoggerTest.cpp)

add_executable(LogSinkTest log/LogSinkTest.cpp)

# coro
add_executable(TaskTest coro/TaskTest.cpp)
add_test(NAME TaskTest COMMAND TaskTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

# net
add_executable(AcceptorTest net/AcceptorTest.cpp)

add_executable(IpAddressTest net/IpAddressTest.cpp)
add_test(name IpAddressTest COMMAND IpAddressTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable(ConnectTest net/ConnectTest.cpp)

add_executable(ReadWriteTest net/ReadWriteTest.cpp)

add_executable(IoContextPoolTest net/IoContextPoolTest.cpp)

add_executable(ReusePortTest net/ReusePortTest.cpp)

# util
add_executable(ConfigTest util/ConfigTest.cpp)
add_test(NAME ConfigTest COMMAND ConfigTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR})


add_executable(IntWriterTest util/IntWriterTest.cpp)
add_test(NAME IntWriterTest COMMAND IntWriterTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR})