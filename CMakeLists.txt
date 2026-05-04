cmake_minimum_required(VERSION 3.16)
project(spsc_queue CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra -Wpedantic)
add_compile_options("$<$<CONFIG:Release>:-O3;-march=native>")

add_library(spsc_queue INTERFACE)
target_include_directories(spsc_queue INTERFACE include/)

enable_testing()
add_subdirectory(tests)
add_subdirectory(bench)
