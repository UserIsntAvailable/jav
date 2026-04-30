#include <cassert>
#include <cctype>
#include <cstddef>
#include <iterator>
#include <optional>
#include <print>
#include <ranges>
#include <string_view>
#include <utility>
#include <vector>

// XXX(Unavailable): constexpr all things

// namespace Tokenizer

struct Value
{
  enum struct Kind
  {
    // Delimited by `{}`
    OBJECT,
    // Delimited by `""`
    STRING,
  };

  // If `kind` is `STRING`, this points to the exact substring of the value,
  // (e.g "value"), however, if `kind` is `OBJECT`, then the start of value
  // points to '{', but it doesn't end at the matching '}'.
  std::string_view data;
  Kind             kind;
};

#define TRY(EXPR, LOC)              \
  do {                              \
    if (auto _opt_ = (EXPR); _opt_) \
    {                               \
      LOC = *_opt_;                 \
    }                               \
    else                            \
    {                               \
      return std::nullopt;          \
    }                               \
  } while (0)

#define TRY_(EXPR)             \
  do {                         \
    if (!((EXPR).has_value())) \
    {                          \
      return std::nullopt;     \
    }                          \
  } while (0)

auto SplitAt(std::string_view& str, std::size_t i)
  -> std::optional<std::string_view>
{
  if (i > str.size()) return std::nullopt; // XXX(Unavailable): diagnostics

  auto left = str.substr(0, i);
  str = str.substr(i);
  return {left};
}

auto IsWhitespace(char s) -> bool
{
  return s == ' ' || s == '\f' || s == '\n' || s == '\r' || s == '\t' || s == '\v';
}

auto SkipWhitespace(std::string_view& str) -> std::optional<std::monostate>
{
  if (str.empty()) return std::nullopt; // XXX(Unavailable): diagnostic

  while (IsWhitespace(str[0]))
  {
    str = str.substr(1);
    if (str.empty()) return std::nullopt; // XXX(Unavailable): diagnostic
  }

  return std::monostate{};
}

auto ParseString(std::string_view& str) -> std::optional<std::string_view>
{
  if (str.empty()) return std::nullopt; // XXX(Unavailable): diagnostic
  if (str[0] != '"') return std::nullopt; // XXX(Unavailable): diagnostic

  std::size_t i = 0;
  while (++i)
  {
    if (i >= str.size()) return std::nullopt; // XXX(Unavailable): diagnostic
    if (str[i] == '"') break;
  }

  return SplitAt(str, i + 1); // XXX(Unavailable): diagnostics
}

auto NextKeyValue(std::string_view& str)
  -> std::optional<std::pair<std::string_view, Value>>
{
  TRY_(SkipWhitespace(str)); // XXX(Unavailable): diagnostic

  if (str[0] != '"') return std::nullopt; // XXX(Unavailable): diagnostic

  std::string_view key;
  TRY(ParseString(str), key); // XXX(Unavailable): diagnostic

  TRY_(SkipWhitespace(str)); // XXX(Unavailable): diagnostic

  std::string_view value;
  Value::Kind valueKind;
  if (str[0] == '"')
  {
    TRY(ParseString(str), value); // XXX(Unavailable): diagnostic
    valueKind = Value::Kind::STRING;
  }
  else if (str[0] == '{')
  {
    value = str;
    valueKind = Value::Kind::OBJECT;
  }
  else
  {
    return std::nullopt; // XXX(Unavailable): diagnostic
  }

  return std::optional{std::pair{key, Value{value, valueKind}}};
}

auto SubstrUntilClosingPair(std::string_view& str) -> std::optional<std::string_view>
{
  if (str.empty()) return std::nullopt; // XXX(Unavailable): diagnostic
  if (str[0] != '{') return std::nullopt; // XXX(Unavailable): diagnostic

  // XXX(Unavailable): better name?
  std::size_t counter = 0;

  std::size_t i = 0;
  do
  {
    if (i >= str.size()) return std::nullopt; // XXX(Unavailable): diagnostic

    // XXX(Unavailable): switch statement?
    // FIXME(Unavailable): Only checking '{' '}' is terrible; semmantic parsing
    // is needed.

    if      (str[i] == '{') counter++;
    else if (str[i] == '}') counter--;

    if (counter == 0) break;
  }
  while (++i);

  return SplitAt(str, i + 1); // XXX(Unavailable): diagnostics
}

// Visit the contents of a `vdf` file contents.
//
// DOCS(Unavailable): Examples
//
// XXX(Unavailable): `bool strict`
template<std::ranges::input_range R>
auto VisitVDF(std::string_view vdf, const R& keys) -> std::optional<std::string_view>
{
  if (std::ranges::empty(keys)) return {vdf};

  auto keysBegin = std::ranges::begin(keys);
  const auto keysEnd = std::ranges::end(keys);

  std::string_view curObj = vdf;
  for (;;)
  {
    auto keyValue = NextKeyValue(curObj);

    if (!keyValue.has_value()) return std::nullopt;

    const auto [key, value] = *keyValue;
    const auto keyStripped = key.substr(1, key.size() - 2);

    if (keyStripped == *keysBegin)
    {
      keysBegin = std::next(keysBegin);
      if (keysBegin == keysEnd)
      {
        // XXX(Unavailable): std::variant?
        if (value.kind == Value::Kind::OBJECT)
        {
          return SubstrUntilClosingPair(curObj); // XXX(Unavailable): diagnostics
        }
        else if (value.kind == Value::Kind::STRING)
        {
          return {value.data};
        }

        std::unreachable(); // only OBJECT and STRING
      }

      if (value.kind != Value::Kind::OBJECT) return std::nullopt; // XXX(unavailable): diagnostics
      curObj = value.data.substr(1); // Skip '{'
    }
    else
    {
      // NextKeyValue() can only get the next key value pair if the kind of the
      // previous parsed value is `STRING`. In order to find the next key after
      // a value of kind `OBJECT`, you need to find the matching closing '}',
      // which is currently not handled by the current implementation of
      // NextKeyValue().
      //
      // A possible change for NextKeyValue() would be to give it a previous
      // key value pair hint, indicating the kind of the previous value.
      if (value.kind == Value::Kind::OBJECT)
      {
        // XXX(Unavailable): A possible optimisation here would be to early
        // return if the current object is root, since we know that root only
        // has 1 key.

        TRY_(SubstrUntilClosingPair(curObj)); // XXX(Unavailable): diagnostics
      }
    }
  }

  std::unreachable();
}

// main

auto main() -> int
{
  using vector_sv = std::vector<std::string_view>;
  std::ranges::empty_view<std::string_view> empty_view;

  // Idempotent
  assert(VisitVDF(R"("")",     empty_view) == R"("")");
  assert(VisitVDF(R"("a" {})", empty_view) == R"("a" {})");

  // Root
  assert(VisitVDF(R"("a" {})", vector_sv{"a"}) == "{}");

  // Inner
  auto abclibjav = R"("a" { "b" {} "c" { "lib" "jav" } })";
  assert(VisitVDF(abclibjav, vector_sv{"a", "b"}) == R"({})");
  assert(VisitVDF(abclibjav, vector_sv{"a", "c"}) == R"({ "lib" "jav" })");

  // XXX(Unavailable): Passes for now, shouldn't with `strict = true`.
  assert(VisitVDF(R"("a" {{}})"            , empty_view)          == R"("a" {{}})"); // Parser doesn't validate the whole input.
  assert(VisitVDF(R"("a" {} "b" {})"       , vector_sv{"b"})      == "{}"); // Top-level doesn't allow multiple keys.
  assert(VisitVDF(R"("a" { "b" {}} {{}} })", vector_sv{"a", "b"}) == "{}"); // Parser doesn't validate the whole input.
  // XXX(Unavailable): This should probably also fail with `strict = false`.
  assert(VisitVDF(R"("a" { "b" {"a"} })"   , vector_sv{"a", "b"}) == R"({"a"})"); // Object parsing doesn't validate contents inside "{}".

  // XXX(Unavailable): Test steam configuration files.

  std::println("All tests has passed!");

  return 0;
}
