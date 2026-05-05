#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_SUPER_FAST_ASSERTS
#include "doctest.h"
#include "../src/main.cpp"

#define STATIC_CHECK(...) static_assert(__VA_ARGS__); CHECK(__VA_ARGS__);
#define STATIC_CHECK_EQ(ARG1, ARG2) static_assert(ARG1 == ARG2); CHECK_EQ(ARG1, ARG2);

using vector_sv = std::vector<std::string_view>;
using empty_view = std::ranges::empty_view<std::string_view>;

TEST_CASE("idempotent")
{
  STATIC_CHECK_EQ(
    VisitVDF(R"("")", empty_view{}),
    R"("")"
  );

  STATIC_CHECK_EQ(
    VisitVDF(R"("a" {})", empty_view{}),
    R"("a" {})"
  );
}

TEST_CASE("top-level")
{
  STATIC_CHECK_EQ(
    VisitVDF(R"("a" {})", vector_sv{"a"}),
    "{}"
  );
}

TEST_CASE("inner")
{
  constexpr auto abclib = R"("a" { "b" {} "c" { "lib" "jav" } })";

  STATIC_CHECK_EQ(
    VisitVDF(abclib, vector_sv{"a", "b"}),
    "{}"
  );

  STATIC_CHECK_EQ(
    VisitVDF(abclib, vector_sv{"a", "c"}),
    R"({ "lib" "jav" })"
  );

  STATIC_CHECK_EQ(
    VisitVDF(R"("a" { "a" { "a" "a" }})", vector_sv{"a", "a", "a"}),
    R"("a")"
  );
}

TEST_CASE("strict=false")
{
  STATIC_CHECK_EQ(
    VisitVDF(R"("a" {{}})" , empty_view{}),
    R"("a" {{}})"
  ); // Parser doesn't validate the whole input.

  STATIC_CHECK_EQ(
    VisitVDF(R"("a" { "b" {}} {{}} })", vector_sv{"a", "b"}),
    "{}"
  ); // Parser doesn't validate the whole input.

  STATIC_CHECK_EQ(
    VisitVDF(R"("a" {} "b" {})", vector_sv{"b"}),
    "{}"
  ); // Top-level doesn't allow multiple keys.

  // XXX(Unavailable): This should probably also fail with `strict = false`.
  STATIC_CHECK_EQ(
    VisitVDF(R"("a" { "b" {"a"} })", vector_sv{"a", "b"}),
    R"({"a"})"
  ); // Object parsing doesn't validate contents inside "{}".
}

TEST_CASE("string escaping")
{
  STATIC_CHECK_EQ(
    VisitVDF(R"("\"hello\"" { "\"world\"" "GO\"NE" } )", vector_sv{"\\\"hello\\\"", "\\\"world\\\""}),
    R"("GO\"NE")"
  );
}

TEST_CASE("object scoping")
{
  CHECK(!VisitVDF(R"("a" { "b" { "c" "c" } "d" "d" })", vector_sv{"a", "b", "d"}).has_value());
}
