cmake_minimum_required(VERSION 3.14)
project(ferrugo-core)

enable_testing()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-export-dynamic")

include(dependencies.cmake)

add_subdirectory(src)

# add_subdirectory(tests)