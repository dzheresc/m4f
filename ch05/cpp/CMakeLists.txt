cmake_minimum_required(VERSION 3.20)
project(m4f_ch05 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_library(m4f_ch05 src/report.cc)
target_include_directories(m4f_ch05 PUBLIC include)
target_compile_options(m4f_ch05 PUBLIC -Wall -Wextra -Wpedantic)

add_executable(ch05 src/main.cc)
target_link_libraries(ch05 PRIVATE m4f_ch05)

enable_testing()
add_executable(ch05_tests tests/ch05_test.cc)
target_link_libraries(ch05_tests PRIVATE m4f_ch05)
add_test(NAME ch05_tests COMMAND ch05_tests)
