cmake_minimum_required(VERSION "3.8")

# decline in-source build
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "In-source builds are not allowed.")
endif()

# project information
project(
  cpp_bench
  VERSION "1.1.1"
  DESCRIPTION "A benchmarker library for C++ language."
  LANGUAGES "CXX"
)

# define source directories
set(CPP_BENCH_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

#--------------------------------------------------------------------------------------#
# Build library
#--------------------------------------------------------------------------------------#

if(NOT TARGET cpp_bench::cpp_bench)
  add_library(cpp_bench INTERFACE)
  add_library(cpp_bench::cpp_bench ALIAS cpp_bench)
  target_compile_features(cpp_bench INTERFACE
    "cxx_std_17"
  )
  target_include_directories(cpp_bench INTERFACE
    "${CPP_BENCH_SOURCE_DIR}/include"
  )
  target_link_libraries(cpp_bench INTERFACE
    pthread
  )
endif()

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
