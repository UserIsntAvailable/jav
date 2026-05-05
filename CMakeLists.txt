cmake_minimum_required(VERSION 3.23)

project(jav)

# This is ON by default with `include(CTest)`.
option(BUILD_TESTING "Build tests" OFF)
include(CTest)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
