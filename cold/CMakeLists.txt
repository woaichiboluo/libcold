set(
    SOURCE
    time/Time.cpp
    thread/Thread.cpp
    log/Logger.cpp
    log/LogFormatter.cpp
)

add_library(cold ${SOURCE})