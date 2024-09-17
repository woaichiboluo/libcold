include_directories(${CMAKE_SOURCE_DIR})

add_executable(TimeTest TimeTest.cpp)
add_test(NAME TimeTest COMMAND TimeTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable(LogFormatterTest LogFormatterTest.cpp)
add_test(NAME LogFormatterTest COMMAND LogFormatterTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable(LoggerTest LoggerTest.cpp)

add_executable(LogSinkTest LogSinkTest.cpp)

add_executable(TaskTest TaskTest.cpp)
add_test(NAME TaskTest COMMAND TaskTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable(IoAwaitableTest IoAwaitableTest.cpp)

add_executable(IpAddressTest IpAddressTest.cpp)
add_test(name IpAddressTest COMMAND IpAddressTest WORKING_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable(TimerTest TimerTest.cpp)

add_executable(TimeoutAndSleepTest TimeoutAndSleepTest.cpp)