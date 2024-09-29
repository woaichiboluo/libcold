cmake_minimum_required(VERSION 3.0)

project(libcold)

set(CMAKE_EXPORT_COMPILER_COMMANDS ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter -Wold-style-cast")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wconversion -Wshadow -Woverloaded-virtual")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFMT_HEADER_ONLY")


if("${CMAKE_BUILD_TYPE}" STREQUAL Debug)
    set(BUILD_TEST ON)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
    link_libraries(asan)
else()
    set(CMAKE_BUILD_TYPE Release)
endif()

# find openssl
# find_package(OpenSSL)
# if(OPENSSL_FOUND)
#     message("Found OpenSSL: ${OPENSSL_INCLUDE_DIR}")
#     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCOLD_NET_ENABLE_SSL")
#     set(BUILD_SSL_MODE ON)
# else()
#     set(BUILD_SSL_MODE OFF)
#     message("Not found OpenSSL")
# endif()

#find protobuf
# find_package(protobuf CONFIG)
# if(protobuf_FOUND)
#     message("Found Protobuf")
#     include_directories(${protobuf_INCLUDE_DIR})
#     set(BUILD_PROTOBUF_MODE ON)
# else()
#     set(BUILD_PROTOBUF_MODE OFF)
#     message("Not found Protobuf")
# endif()

message("CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message("CMAKE_C_COMPILER: ${CMAKE_CXX_COMPILER_ID}")
message("CMAKE_CXX_COMPILER: ${CMAKE_CXX_COMPILER_ID}")
message("CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
message("CMAKE_C_FLAGS: ${CMAKE_C_FLAGS}")

# include_directories(${PROJECT_SOURCE_DIR})
# include_directories(${PROJECT_SOURCE_DIR}/third_party/uuid)


# add_subdirectory(third_party/llhttp)
# add_subdirectory(cold)
add_subdirectory(examples)

if(${BUILD_TEST})
    enable_testing()
    add_subdirectory(test)
    message("BUILD_TEST: ON")
endif()