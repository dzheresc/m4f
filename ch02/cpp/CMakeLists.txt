cmake_minimum_required(VERSION 3.20)
project(m4f_ch02 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_library(m4f_ch02 src/report.cpp)
target_include_directories(m4f_ch02 PUBLIC include)
target_compile_options(m4f_ch02 PUBLIC -Wall -Wextra -Wpedantic)

add_executable(ch02 src/main.cpp)
target_link_libraries(ch02 PRIVATE m4f_ch02)

enable_testing()
add_executable(ch02_tests tests/test_ch02.cpp)
target_link_libraries(ch02_tests PRIVATE m4f_ch02)
add_test(NAME ch02_tests COMMAND ch02_tests)
