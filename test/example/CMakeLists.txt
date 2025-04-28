cmake_minimum_required(VERSION "3.11")

# decline in-source build
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "In-source builds are not allowed.")
endif()

# project information
project(
  example_bench
  VERSION "1.0.0"
  DESCRIPTION "A sample benchmark program."
  LANGUAGES "CXX"
)

# enable features
include(FetchContent)

if(NOT TARGET dbgroup::${PROJECT_NAME})
  #----------------------------------------------------------------------------#
  # Configurations
  #----------------------------------------------------------------------------#

  # use our utility
  FetchContent_Declare(
    cpp-utility
    GIT_REPOSITORY "https://github.com/dbgroup-nagoya-u/cpp-utility.git"
    GIT_TAG "6487d3fe2ab4e70b5ab5cee7742cccf92bb5f072"
  )
  FetchContent_MakeAvailable(cpp-utility)

  # use our benchmark utility
  # FetchContent_Declare(
  #   cpp-utility
  #   GIT_REPOSITORY "https://github.com/dbgroup-nagoya-u/cpp-benchmark.git"
  #   GIT_TAG "..."
  # )
  # FetchContent_MakeAvailable(cpp-benchmark)

  #----------------------------------------------------------------------------#
  # Build benchmark target for unit tests
  #----------------------------------------------------------------------------#

  # build as library for testing
  add_library(${PROJECT_NAME} STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/target.cpp"
  )
  add_library(dbgroup::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
  target_compile_features(${PROJECT_NAME} PRIVATE
    "cxx_std_20"
  )
  target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<STREQUAL:"${CMAKE_BUILD_TYPE}","Release">:-march=native>
    $<$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">:-g3>
    -Wall
    -Wextra
  )
  target_include_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
  )
  target_link_libraries(${PROJECT_NAME} PUBLIC
    dbgroup::cpp_utility
    dbgroup::cpp_bench
  )
endif()

#------------------------------------------------------------------------------#
# Build Benchmark
#------------------------------------------------------------------------------#

# define function to add unit tests in the same format
function(ADD_BENCHMARK BENCHMARK_TARGET)
  add_executable(${BENCHMARK_TARGET}
    "${CMAKE_CURRENT_SOURCE_DIR}/src/${BENCHMARK_TARGET}.cpp"
  )
  target_compile_features(${BENCHMARK_TARGET} PRIVATE
    "cxx_std_20"
  )
  target_compile_options(${BENCHMARK_TARGET} PRIVATE
    $<$<STREQUAL:"${CMAKE_BUILD_TYPE}","Release">:-march=native>
    $<$<STREQUAL:"${CMAKE_BUILD_TYPE}","Debug">:-g3>
    -Wall
    -Wextra
  )
  target_include_directories(${BENCHMARK_TARGET} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
  )
  target_link_libraries(${BENCHMARK_TARGET} PRIVATE
    dbgroup::${PROJECT_NAME}
  )
endfunction()

# add executables
ADD_BENCHMARK("run_bench")
