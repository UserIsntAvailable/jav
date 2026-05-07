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
    jav::findKey(R"("")", empty_view{}),
    R"("")"
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

// FIXME(Unavailable): Drop the idea of `strict`. VisitVDF _by default_ should
// validate the whole input string; as such, all checks in this TEST_CASE should
// considered invalid.
TEST_CASE("strict=false")
{
  STATIC_CHECK_EQ(
    jav::findKey(R"("a" {{}})" , empty_view{}),
    R"("a" {{}})"
  ); // Parser doesn't validate the whole input.

  STATIC_CHECK_EQ(
    jav::findKey(R"("a" { "b" {}} {{}} })", vector_sv{"a", "b"}),
    "{}"
  ); // Parser doesn't validate the whole input.

  // XXX(Unavailable): This should probably also fail with `strict = false`.
  STATIC_CHECK_EQ(
    jav::findKey(R"("a" { "b" {"a"} })", vector_sv{"a", "b"}),
    R"({"a"})"
  ); // Object parsing doesn't validate contents inside "{}".
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
  // XXX(Unavailable): Why this can't `STATIC_CHECK`?
  CHECK(!jav::findKey(R"("a" { "b" { "c" "c" } "d" "d" })", vector_sv{"a", "b", "d"}).has_value());
}

// FIXME(Unavailable): Comment parsing.
// FIXME(Unavailable): You can have duplicated keys; last key should win (it is
// what most implementations do), so VisitVDF should not exit as soon as possible.

// XXX(Unavailable): Invalid file tests.
// XXX(Unavailable): Inconsistent whitespace tests.
