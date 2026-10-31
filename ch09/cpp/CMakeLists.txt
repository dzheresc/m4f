cmake_minimum_required(VERSION 3.20)
project(m4f_ch09 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_library(m4f_ch09 src/report.cc)
target_include_directories(m4f_ch09 PUBLIC include)
target_compile_options(m4f_ch09 PUBLIC -Wall -Wextra -Wpedantic)

add_executable(ch09 src/main.cc)
target_link_libraries(ch09 PRIVATE m4f_ch09)

enable_testing()
add_executable(ch09_tests tests/ch09_test.cc)
target_link_libraries(ch09_tests PRIVATE m4f_ch09)
add_test(NAME ch09_tests COMMAND ch09_tests)
