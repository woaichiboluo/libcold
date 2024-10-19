cmake_minimum_required(VERSION 3.0)

project(libcold)

set(CMAKE_EXPORT_COMPILER_COMMANDS ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -g")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter -Wold-style-cast")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wconversion -Wshadow -Woverloaded-virtual")

option(BUILD_TEST "Build cold tests." ON)
option(BUILD_EXAMPLES "Build cold examples." ON)


if("${CMAKE_BUILD_TYPE}" STREQUAL Debug)
    # Check GCC
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
        link_libraries(asan)
    endif()
else()
    set(CMAKE_BUILD_TYPE Release)
endif()

# find openssl
find_package(OpenSSL)
if(OPENSSL_FOUND)
    set(COLD_SSL_AVAILABLE ON)
else()
    set(COLD_SSL_AVAILABLE OFF)
endif()

# http 
find_library(LIBUUID uuid)
find_package(llhttp)

if(LIBUUID AND llhttp_FOUND)
    set(COLD_HTTP_AVAILABLE ON)
else()
    set(COLD_HTTP_AVAILABLE OFF)
endif()


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

if(${BUILD_EXAMPLES})
    add_subdirectory(examples)
endif()

if(${BUILD_TEST})
    enable_testing()
    add_subdirectory(test)
endif()


message(STATUS "")
message(STATUS "")
message(STATUS "libcold configure summary:")
message(STATUS "")
message(STATUS "  CMAKE_C_FLAGS .....................: ${CMAKE_C_FLAGS}")
message(STATUS "  CMAKE_CXX_FLAGS ...................: ${CMAKE_CXX_FLAGS}")
message(STATUS "  CMake build type ..................: ${CMAKE_BUILD_TYPE}")
message(STATUS "  CMAKE_CXX_COMPILER ................: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "  Install prefix ....................: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  BUILD_TEST ........................: ${BUILD_TEST}")
message(STATUS "  BUILD_EXAMPLES ....................: ${BUILD_TEST}")
message(STATUS "  COLD_SSL_AVAILABLE ................: ${COLD_SSL_AVAILABLE}")
message(STATUS "  COLD_HTTP_AVAILABLE ...............: ${COLD_HTTP_AVAILABLE}")
message(STATUS "")


