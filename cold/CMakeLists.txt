set(
    SOURCE
    time/Time.cpp
    thread/Thread.cpp
    log/Logger.cpp
    log/LogFormatter.cpp
    coro/IoWatcher.cpp
    coro/IoService.cpp
    coro/IoServicePool.cpp
    net/IpAddress.cpp
    net/BasicSocket.cpp
    net/Acceptor.cpp
    net/http/HttpRequestParser.cpp
    net/http/HttpRequest.cpp
    time/Timer.cpp
    time/TimerQueue.cpp
)

add_library(cold SHARED ${SOURCE})
target_link_libraries(cold PUBLIC llhttp)