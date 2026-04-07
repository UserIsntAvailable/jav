cmake_minimum_required(VERSION 3.23)

project(jav)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_BUILD_TYPE Debug)

add_executable(jav)
target_compile_options(jav PRIVATE
  -Wall
  -Wextra
  -Wpedantic
  -Wconversion
  -Wold-style-cast
  -Wunused
)

target_sources(jav PRIVATE src/main.cpp)

# target_link_libraries(jav)
