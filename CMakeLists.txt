cmake_minimum_required(VERSION "3.8")

# decline in-source build
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "In-source builds are not allowed.")
endif()

# project information
project(
  cpp_bench
  VERSION "2.3.0"
  DESCRIPTION "A benchmarker library for C++ language."
  LANGUAGES "CXX"
)

if(NOT TARGET dbgroup::${PROJECT_NAME})
  # enable features
  include(FetchContent)

  #----------------------------------------------------------------------------#
  # Configurations
  #----------------------------------------------------------------------------#

  # use multi threads
  find_package(Threads)

  # use our utility
  FetchContent_Declare(
    cpp-utility
    GIT_REPOSITORY "https://github.com/dbgroup-nagoya-u/cpp-utility"
    GIT_TAG "32551c6d394e986508c4f4594d2c8cd3bd947132" # 0.19.0
  )
  FetchContent_MakeAvailable(cpp-utility)

  #----------------------------------------------------------------------------#
  # Build target
  #----------------------------------------------------------------------------#

  add_library(${PROJECT_NAME} INTERFACE)
  add_library(dbgroup::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
  target_compile_features(${PROJECT_NAME} INTERFACE
    "cxx_std_20"
  )
  target_include_directories(${PROJECT_NAME} INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
  )
  target_link_libraries(${PROJECT_NAME} INTERFACE
    Threads::Threads
    dbgroup::cpp_utility
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
