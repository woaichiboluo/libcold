set(
    SOURCE
    time/Time.cpp
    thread/Thread.cpp
    log/Logger.cpp
    log/LogFormatter.cpp
    coro/IoWatcher.cpp
    coro/IoService.cpp
)

add_library(cold ${SOURCE})