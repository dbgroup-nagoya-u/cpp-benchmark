cmake_minimum_required(VERSION "3.8")

# decline in-source build
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "In-source builds are not allowed.")
endif()

# project information
project(
  mwcas_bench
  VERSION "0.1.0"
  DESCRIPTION "A template project for C++ language."
  LANGUAGES "CXX"
)

# define source directories
set(MWCAS_BENCH_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

#--------------------------------------------------------------------------------------#
# Set Compile Options
#--------------------------------------------------------------------------------------#

set(
  MWCAS_BENCH_MAX_TARGET_NUM
  "8"
  CACHE STRING
  "The maximum number of target words of MwCAS."
)

#--------------------------------------------------------------------------------------#
# Configure MwCAS
#--------------------------------------------------------------------------------------#

set(MWCAS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/mwcas")

# set MwCAS capacity
set(MWCAS_CAPACITY "${MWCAS_BENCH_MAX_TARGET_NUM}" CACHE STRING "" FORCE)

add_subdirectory("${MWCAS_SOURCE_DIR}")

#--------------------------------------------------------------------------------------#
# Configure PMwCAS
#--------------------------------------------------------------------------------------#

set(PMWCAS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/pmwcas")

# build volatile PMwCAS
set(PMEM_BACKEND "Volatile" CACHE STRING "" FORCE)

# set MwCAS capacity
set(DESC_CAP "${MWCAS_BENCH_MAX_TARGET_NUM}" CACHE STRING "" FORCE)

# prevent building Google libraries
set(GOOGLE_FRAMEWORK OFF CACHE BOOL "" FORCE)

# prevent building unit tests
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_APPS OFF CACHE BOOL "" FORCE)

add_subdirectory("${PMWCAS_SOURCE_DIR}")

#--------------------------------------------------------------------------------------#
# Configure gflags
#--------------------------------------------------------------------------------------#

set(GFLAGS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/gflags")

# prevent building unit tests
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)

add_subdirectory("${GFLAGS_SOURCE_DIR}")

#--------------------------------------------------------------------------------------#
# Configure cpp-utility
#--------------------------------------------------------------------------------------#

set(CPP_UTILITY_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/cpp-utility")

add_subdirectory("${CPP_UTILITY_SOURCE_DIR}")

#--------------------------------------------------------------------------------------#
# Build Benchmark
#--------------------------------------------------------------------------------------#

# build executable
add_executable(
  mwcas_bench
  "${MWCAS_BENCH_SOURCE_DIR}/src/mwcas_bench.cpp"
)
target_compile_features(
  mwcas_bench PRIVATE
  "cxx_std_17"
)
target_compile_options(
  mwcas_bench PRIVATE
  -Wall
  -Wextra
  $<$<STREQUAL:${CMAKE_BUILD_TYPE},"Release">:"-O2 -march=native">
  $<$<STREQUAL:${CMAKE_BUILD_TYPE},"RelWithDebInfo">:"-g3 -Og -pg">
  $<$<STREQUAL:${CMAKE_BUILD_TYPE},"Debug">:"-g3 -O0 -pg">
)
target_compile_definitions(
  mwcas_bench PRIVATE
  MWCAS_BENCH_MAX_TARGET_NUM=${MWCAS_BENCH_MAX_TARGET_NUM}
)
target_include_directories(
  mwcas_bench PRIVATE
  "${PMWCAS_SOURCE_DIR}/"
  "${PMWCAS_SOURCE_DIR}/src"
  "${PMWCAS_SOURCE_DIR}/include"
)
target_link_libraries(
  mwcas_bench PRIVATE
  mwcas
  pmwcas_static
  rt
  gflags
  cpp_utility
)

#--------------------------------------------------------------------------------------#
# Build unit tests
#--------------------------------------------------------------------------------------#

option(MWCAS_BENCH_BUILD_TESTS "Build unit tests" OFF)
if(${MWCAS_BENCH_BUILD_TESTS})
  # enable CTest
  enable_testing()
  # include unit tests
  add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/test")
endif()
