cmake_minimum_required(VERSION 3.20)
project(m4f_ch11 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_library(m4f_ch11 src/report.cpp)
target_include_directories(m4f_ch11 PUBLIC include)
target_compile_options(m4f_ch11 PUBLIC -Wall -Wextra -Wpedantic)

add_executable(ch11 src/main.cpp)
target_link_libraries(ch11 PRIVATE m4f_ch11)

enable_testing()
add_executable(ch11_tests tests/test_ch11.cpp)
target_link_libraries(ch11_tests PRIVATE m4f_ch11)
add_test(NAME ch11_tests COMMAND ch11_tests)
