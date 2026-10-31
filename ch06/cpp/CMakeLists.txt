cmake_minimum_required(VERSION 3.20)
project(m4f_ch06 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_library(m4f_ch06 src/report.cc)
target_include_directories(m4f_ch06 PUBLIC include)
target_compile_options(m4f_ch06 PUBLIC -Wall -Wextra -Wpedantic)

add_executable(ch06 src/main.cc)
target_link_libraries(ch06 PRIVATE m4f_ch06)

enable_testing()
add_executable(ch06_tests tests/ch06_test.cc)
target_link_libraries(ch06_tests PRIVATE m4f_ch06)
add_test(NAME ch06_tests COMMAND ch06_tests)
