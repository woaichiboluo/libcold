cmake_minimum_required(VERSION 3.0)

project(libcold)

message("CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message("CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")

set(CMAKE_EXPORT_COMPILER_COMMANDS ON)
set(BUILD_TESTING ON)

add_compile_options(-std=c++20 -g)
add_compile_options(-Wall -Wextra -Werror)
add_compile_options(-Wno-unused-parameter -Wno-old-style-cast)
add_compile_options(-Wconversion -Wshadow -Woverloaded-virtual)

if(${CMAKE_BUILD_TYPE} STREQUAL Debug)
    add_compile_options(-fsanitize=address)
    link_libraries(asan)
endif()

if(${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
    add_compile_options(-Wthread-safety)
endif()

if(${BUILD_TESTING})
    enable_testing()
endif()

include_directories(${PROJECT_SOURCE_DIR})

add_subdirectory(cold)
add_subdirectory(test)