#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_SUPER_FAST_ASSERTS
#include "doctest.h"
#include <vector>
#include <jav/jav.hpp>

#define STATIC_CHECK(...) static_assert(__VA_ARGS__); CHECK(__VA_ARGS__);
#define STATIC_CHECK_EQ(ARG1, ARG2) STATIC_CHECK(ARG1 == ARG2);

using vector_sv = std::vector<std::string_view>;
using empty_view = std::ranges::empty_view<std::string_view>;

TEST_CASE("idempotent")
{
  STATIC_CHECK_EQ(
    jav::findKey("", empty_view{}),
    ""
  );

  STATIC_CHECK_EQ(
    jav::findKey(R"("a" "b")", empty_view{}),
    R"("a" "b")"
  );

  STATIC_CHECK_EQ(
    jav::findKey(R"("a" {})", empty_view{}),
    R"("a" {})"
  );
}

TEST_CASE("top-level")
{
  STATIC_CHECK_EQ(
    jav::findKey(R"("a" {})", vector_sv{"a"}),
    "{}"
  );

  STATIC_CHECK_EQ(
    jav::findKey(R"("a" {} "b" {})", vector_sv{"b"}),
    "{}"
  );

  STATIC_CHECK_EQ(
    jav::findKey(R"("a" {} "b" { "a" {} } "c" "skip")", vector_sv{"c"}),
    R"("skip")"
  );
}

TEST_CASE("inner")
{
  constexpr auto abclib = R"("a" { "b" {} "c" { "lib" "jav" } })";

  STATIC_CHECK_EQ(
    jav::findKey(abclib, vector_sv{"a", "b"}),
    "{}"
  );

  STATIC_CHECK_EQ(
    jav::findKey(abclib, vector_sv{"a", "c"}),
    R"({ "lib" "jav" })"
  );

  STATIC_CHECK_EQ(
    jav::findKey(R"("a" { "a" { "a" "a" }})", vector_sv{"a", "a", "a"}),
    R"("a")"
  );
}

TEST_CASE("string escaping")
{
  STATIC_CHECK_EQ(
    jav::findKey(R"("\"hello\"" { "\"world\"" "GO\"NE" } )", vector_sv{"\\\"hello\\\"", "\\\"world\\\""}),
    R"("GO\"NE")"
  );
}

TEST_CASE("object scoping")
{
  STATIC_CHECK_EQ(
    jav::findKey(R"("a" { "b" { "c" "c" } "d" "d" })", vector_sv{"a", "b", "d"}),
    std::nullopt
  );
}

TEST_CASE("comment")
{
  STATIC_CHECK_EQ(
    jav::findKey(R"(// Hello, World!"abc" {})", vector_sv{"abc"}),
    "{}"
  ); // Before key value

  STATIC_CHECK_EQ(
    jav::findKey(R"("def" {}// Goodbye, World!)", vector_sv{"def"}),
    "{}"
  ); // After key value

  STATIC_CHECK_EQ(
    jav::findKey(R"("abc" {}// MITM"def" {})", vector_sv{"def"}),
    "{}"
  ); // In between key values

  STATIC_CHECK_EQ(
    jav::findKey(R"("ghi" //Why is this allowed? "jkl")", vector_sv{"ghi"}),
    R"("jkl")"
  ); // After key

  STATIC_CHECK_EQ(
    jav::findKey(R"(// Returned as is)", empty_view{}),
    "// Returned as is"
  ); // Comments can be terminated with EOF
}

TEST_CASE("duplicate keys")
{
  STATIC_CHECK_EQ(
    jav::findKey(R"("a" "b" "a" "c")", vector_sv{"a"}),
    R"("c")"
  ); // Same object

  STATIC_CHECK_EQ(
    jav::findKey(R"("a" { "b" { "c" { "d" "d" "d" "e" } } "b" { "c" { "d" "f" } } } )", vector_sv{"a", "b", "c", "d"}),
    R"("f")"
  ); // Different object
}

TEST_CASE("inconsistent whitespace")
{
  STATIC_CHECK_EQ(
    jav::findKey("\f\"a\"\t\f\n{\"b\"\f{\n\"c\"\v\"s\"}}\n\"c\"\"c\"", vector_sv{"c"}),
    R"("c")"
  );
}

TEST_CASE("invalid")
{
  // XXX(Unavailable): Make these STATIC_CHECK
  // FIXME(Unavailable): Actually validate that errors have the correct spans.

  CHECK(!jav::findKey(R"("")", empty_view{}).has_value());
  CHECK(!jav::findKey(R"("a" {{}})" , empty_view{}).has_value());
  CHECK(!jav::findKey(R"("a" { "b" {}} {{}} })", vector_sv{"a", "b"}).has_value());
  CHECK(!jav::findKey(R"("a" { "b" {"a"} })", vector_sv{"a", "b"}).has_value());
}

// XXX(Unavailable): fuzzing tests
