cmake_minimum_required(VERSION 3.20)
project(m4f_ch02 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_library(m4f_ch02 src/report.cc)
target_include_directories(m4f_ch02 PUBLIC include)
target_compile_options(m4f_ch02 PUBLIC -Wall -Wextra -Wpedantic)

add_executable(ch02 src/main.cc)
target_link_libraries(ch02 PRIVATE m4f_ch02)

enable_testing()
add_executable(ch02_tests tests/ch02_test.cc)
target_link_libraries(ch02_tests PRIVATE m4f_ch02)
add_test(NAME ch02_tests COMMAND ch02_tests)

# Microbenchmarks (Google Benchmark). Off by default so that a plain build
# needs no network: configure with -DM4F_BUILD_BENCHMARKS=ON, or use
# ../bench.sh. An installed Google Benchmark is used if CMake can find one;
# otherwise a pinned release is downloaded.
option(M4F_BUILD_BENCHMARKS "Build the ch02_benchmark executable" OFF)
if(M4F_BUILD_BENCHMARKS)
  find_package(benchmark 1.8 QUIET)
  if(NOT benchmark_FOUND)
    include(FetchContent)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(benchmark
      GIT_REPOSITORY https://github.com/google/benchmark.git
      GIT_TAG v1.9.1
      GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(benchmark)
  endif()
  add_executable(ch02_benchmark
    benchmarks/interest_benchmark.cc
    benchmarks/money_market_benchmark.cc
    benchmarks/report_benchmark.cc)
  target_link_libraries(ch02_benchmark PRIVATE m4f_ch02 benchmark::benchmark_main)
endif()
