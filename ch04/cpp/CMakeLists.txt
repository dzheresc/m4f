cmake_minimum_required(VERSION 3.20)
project(m4f_ch04 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_library(m4f_ch04 src/report.cc)
target_include_directories(m4f_ch04 PUBLIC include)
target_compile_options(m4f_ch04 PUBLIC -Wall -Wextra -Wpedantic)

add_executable(ch04 src/main.cc)
target_link_libraries(ch04 PRIVATE m4f_ch04)

enable_testing()
add_executable(ch04_tests tests/ch04_test.cc)
target_link_libraries(ch04_tests PRIVATE m4f_ch04)
add_test(NAME ch04_tests COMMAND ch04_tests)
