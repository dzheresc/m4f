cmake_minimum_required(VERSION 3.20)
project(m4f_ch08 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_library(m4f_ch08 src/report.cc)
target_include_directories(m4f_ch08 PUBLIC include)
target_compile_options(m4f_ch08 PUBLIC -Wall -Wextra -Wpedantic)

add_executable(ch08 src/main.cc)
target_link_libraries(ch08 PRIVATE m4f_ch08)

enable_testing()
add_executable(ch08_tests tests/ch08_test.cc)
target_link_libraries(ch08_tests PRIVATE m4f_ch08)
add_test(NAME ch08_tests COMMAND ch08_tests)
