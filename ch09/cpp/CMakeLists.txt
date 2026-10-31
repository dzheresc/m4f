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

# Microbenchmarks (Google Benchmark). Off by default so that a plain build
# needs no network: configure with -DM4F_BUILD_BENCHMARKS=ON, or use
# ../bench.sh. An installed Google Benchmark is used if CMake can find one;
# otherwise a pinned release is downloaded.
option(M4F_BUILD_BENCHMARKS "Build the ch09_benchmark executable" OFF)
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
  add_executable(ch09_benchmark
    benchmarks/black_scholes_benchmark.cc
    benchmarks/hedging_benchmark.cc
    benchmarks/risk_benchmark.cc
    benchmarks/report_benchmark.cc)
  target_link_libraries(ch09_benchmark PRIVATE m4f_ch09 benchmark::benchmark_main)
endif()
