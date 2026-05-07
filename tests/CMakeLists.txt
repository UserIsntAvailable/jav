enable_testing()

include(FetchContent)
FetchContent_Declare(
  doctest
  GIT_REPOSITORY https://github.com/doctest/doctest
  GIT_TAG v2.5.3
)
FetchContent_MakeAvailable(doctest)

add_executable(tests test_lib.cpp)
target_link_libraries(tests PRIVATE
  jav::jav
  doctest::doctest
)

target_compile_options(tests PRIVATE
  -Wall
  -Wextra
  -Wconversion
  -Wold-style-cast
  -Wunused

  -Wpedantic
  -Wno-c++26-extensions
)

include("${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake")
doctest_discover_tests(tests)
