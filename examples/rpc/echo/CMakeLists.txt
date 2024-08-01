message(${CMAKE_CURRENT_SOURCE_DIR})
add_custom_command(
  OUTPUT echo.pb.cc echo.pb.h
  COMMAND protoc
  ARGS --cpp_out ${CMAKE_CURRENT_SOURCE_DIR} echo.proto -I ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS echo.proto
  VERBATIM )

set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/echo.pb.cc PROPERTIES COMPILE_FLAGS "-w")
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/echo.pb.hh PROPERTIES COMPILE_FLAGS "-w")

add_executable(EchoRpcServer Server.cpp ${CMAKE_CURRENT_SOURCE_DIR}/echo.pb.cc)
target_link_libraries(EchoRpcServer PRIVATE cold_rpc)

add_executable(EchoRpcClient Client.cpp ${CMAKE_CURRENT_SOURCE_DIR}/echo.pb.cc)
target_link_libraries(EchoRpcClient PRIVATE cold_rpc)
