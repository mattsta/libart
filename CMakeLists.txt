project(art C)
cmake_minimum_required(VERSION 2.8.9)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_DISABLE_SOURCE_CHANGES  ON)

if (TARGET ${PROJECT_NAME})
    return()
endif()

add_subdirectory(src)
# vi:ai et sw=4 ts=4:
