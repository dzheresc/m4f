cmake_minimum_required(VERSION 3.20)
project(m4f_ch10 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_library(m4f_ch10 src/report.cpp)
target_include_directories(m4f_ch10 PUBLIC include)
target_compile_options(m4f_ch10 PUBLIC -Wall -Wextra -Wpedantic)

add_executable(ch10 src/main.cpp)
target_link_libraries(ch10 PRIVATE m4f_ch10)

enable_testing()
add_executable(ch10_tests tests/test_ch10.cpp)
target_link_libraries(ch10_tests PRIVATE m4f_ch10)
add_test(NAME ch10_tests COMMAND ch10_tests)
