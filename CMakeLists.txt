cmake_minimum_required(VERSION 3.23)
project(jav VERSION 0.1.0 LANGUAGES CXX)

# This is ON by default with `include(CTest)`.
option(BUILD_TESTING "Build tests" OFF)
include(CTest)

option(BUILD_CLI "Build jav CLI" ON)

if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 23)
endif()
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(jav INTERFACE)
add_library(jav::jav ALIAS jav)

target_include_directories(jav INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
)

target_compile_features(jav INTERFACE cxx_std_${CMAKE_CXX_STANDARD})

if(BUILD_CLI)
  add_subdirectory(src)
endif()

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
