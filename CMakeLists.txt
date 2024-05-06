cmake_minimum_required(VERSION 3.0)

project(libcold)


set(CMAKE_EXPORT_COMPILER_COMMANDS ON)

add_compile_options(-std=c++20 -g)
add_compile_options(-Wall -Wextra -Werror)
add_compile_options(-Wno-unused-parameter -Wno-old-style-cast)
add_compile_options(-Wconversion -Wshadow -Woverloaded-virtual)
add_compile_options(-DFMT_HEADER_ONLY)

if("${CMAKE_BUILD_TYPE}" STREQUAL Debug)
    set(BUILD_TEST ON)
    add_compile_options(-fsanitize=address)
    link_libraries(asan)
else()
    set(CMAKE_BUILD_TYPE Release)
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL Clang)
    add_compile_options(-Wthread-safety)
endif()


message("CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message("CMAKE_CXX_COMPILER_ID: ${CMAKE_CXX_COMPILER_ID}")

include_directories(${PROJECT_SOURCE_DIR})

add_subdirectory(cold)
add_subdirectory(examples)

if(${BUILD_TEST})
    add_subdirectory(test)
    enable_testing()
    message("BUILD_TEST: ON")
endif()