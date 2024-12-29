cmake_minimum_required(VERSION "3.8")

# decline in-source build
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "In-source builds are not allowed.")
endif()

# project information
project(
  cpp_bench
  VERSION "1.3.1"
  DESCRIPTION "A benchmarker library for C++ language."
  LANGUAGES "CXX"
)

if(NOT TARGET dbgroup::${PROJECT_NAME})
  #----------------------------------------------------------------------------#
  # Configure system libraries
  #----------------------------------------------------------------------------#

  find_package(Threads)

  #----------------------------------------------------------------------------#
  # Build target
  #----------------------------------------------------------------------------#

  add_library(${PROJECT_NAME} STATIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/component/measurements.cpp"
  )
  add_library(dbgroup::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
  target_compile_features(${PROJECT_NAME} PUBLIC
    "cxx_std_20"
  )
  target_include_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
  )
  target_link_libraries(${PROJECT_NAME} PUBLIC
    Threads::Threads
  )

  #----------------------------------------------------------------------------#
  # Build unit tests
  #----------------------------------------------------------------------------#

  option(CPP_BENCH_BUILD_TESTS "Build unit tests" OFF)
  if(${CPP_BENCH_BUILD_TESTS})
    enable_testing()
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/test")
  endif()
endif()
