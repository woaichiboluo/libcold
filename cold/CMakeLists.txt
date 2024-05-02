set(
    SOURCE
    time/Time.cpp
    thread/Thread.cpp
    log/Logger.cpp
    log/LogFormatter.cpp
    coro/IoWatcher.cpp
    coro/IoService.cpp
    net/IpAddress.cpp
    net/BasicSocket.cpp
    net/Acceptor.cpp
    time/Timer.cpp
    time/TimerQueue.cpp
)

add_library(cold ${SOURCE})