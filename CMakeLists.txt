cmake_minimum_required(VERSION "3.8")

# decline in-source build
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "In-source builds are not allowed.")
endif()

# project information
project(
  cpp_bench
  VERSION "0.1.0"
  DESCRIPTION "A benchmarker library for C++ language."
  LANGUAGES "CXX"
)

# define source directories
set(CPP_BENCH_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(CPP_UTILITY_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/cpp-utility")

#--------------------------------------------------------------------------------------#
# Build library
#--------------------------------------------------------------------------------------#

add_library(cpp_bench INTERFACE)
target_compile_features(
  cpp_bench INTERFACE
  "cxx_std_17"
)
target_compile_options(
  cpp_bench INTERFACE
  -Wall
  -Wextra
  $<$<STREQUAL:${CMAKE_BUILD_TYPE},"Release">:"-O2 -march=native">
  $<$<STREQUAL:${CMAKE_BUILD_TYPE},"RelWithDebInfo">:"-g3 -Og -pg">
  $<$<STREQUAL:${CMAKE_BUILD_TYPE},"Debug">:"-g3 -O0 -pg">
)
target_include_directories(
  cpp_bench INTERFACE
  "${CPP_BENCH_SOURCE_DIR}/include"
  "${CPP_UTILITY_SOURCE_DIR}/include"
)
target_link_libraries(
  cpp_bench INTERFACE
  pthread
)

#--------------------------------------------------------------------------------------#
# Build unit tests
#--------------------------------------------------------------------------------------#

option(CPP_BENCH_BUILD_TESTS "Build unit tests" OFF)
if(${CPP_BENCH_BUILD_TESTS})
  # enable CTest
  enable_testing()
  # include unit tests
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/test")
endif()
