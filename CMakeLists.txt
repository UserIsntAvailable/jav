cmake_minimum_required(VERSION 3.23)
project(jav VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

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
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(jav INTERFACE cxx_std_${CMAKE_CXX_STANDARD})

set(JAV_CMAKE_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/jav")
set(JAV_CONFIG "${PROJECT_BINARY_DIR}/javConfig.cmake")
set(JAV_CONFIG_VERSION "${PROJECT_BINARY_DIR}/javConfigVersion.cmake")

configure_package_config_file(
  cmake/javConfig.cmake.in
  ${JAV_CONFIG}
  INSTALL_DESTINATION ${JAV_CMAKE_DIR}
)
write_basic_package_version_file(
  ${JAV_CONFIG_VERSION}
  COMPATIBILITY SameMinorVersion
)

install(TARGETS jav EXPORT javTargets)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT javTargets
  FILE javTargets.cmake
  NAMESPACE jav::
  DESTINATION ${JAV_CMAKE_DIR}
)
install(FILES
  ${JAV_CONFIG}
  ${JAV_CONFIG_VERSION}
  DESTINATION ${JAV_CMAKE_DIR}
)

if(BUILD_CLI)
  add_subdirectory(src)
endif()

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
