cmake_minimum_required(VERSION 3.12)

project(AlienArena)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}
    CACHE STRING "")

add_subdirectory(source)