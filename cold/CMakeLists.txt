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
    net/http/HttpServer.cpp
    time/Timer.cpp
    time/TimerQueue.cpp
)


add_library(cold ${SOURCE})
target_link_libraries(cold llhttp)

if(OpenSSL_FOUND)
    target_link_libraries(cold ${OPENSSL_LIBRARIES})
endif()

Set(ProtoFolder ${CMAKE_SOURCE_DIR}/cold/net/rpc)

if(protobuf_FOUND)
    # generate rpc.proto
    message("folder:${ProtoFolder}")
    add_custom_command(
    OUTPUT ${ProtoFolder}/rpc.pb.cc ${ProtoFolder}/rpc.pb.hh
    COMMAND protoc
    ARGS --cpp_out ${ProtoFolder} rpc.proto -I ${ProtoFolder}
    DEPENDS ${ProtoFolder}/rpc.proto
    COMMENT ${ProtoFolder}
    )

    set_source_files_properties(rpc.pb.cc PROPERTIES COMPILE_FLAGS "-w")
    set_source_files_properties(rpc.pb.hh PROPERTIES COMPILE_FLAGS "-w")

    set(PROTO_SOURCE 
        ${ProtoFolder}/rpc.pb.cc
        net/rpc/RpcChannel.cpp 
        net/rpc/RpcServer.cpp
    )

    add_library(cold_rpc ${PROTO_SOURCE})
    target_link_libraries(cold_rpc cold protobuf::libprotobuf)
endif()