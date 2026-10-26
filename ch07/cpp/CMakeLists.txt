cmake_minimum_required(VERSION 3.20)
project(m4f_ch07 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_library(m4f_ch07 src/report.cpp)
target_include_directories(m4f_ch07 PUBLIC include)
target_compile_options(m4f_ch07 PUBLIC -Wall -Wextra -Wpedantic)

add_executable(ch07 src/main.cpp)
target_link_libraries(ch07 PRIVATE m4f_ch07)

enable_testing()
add_executable(ch07_tests tests/test_ch07.cpp)
target_link_libraries(ch07_tests PRIVATE m4f_ch07)
add_test(NAME ch07_tests COMMAND ch07_tests)
