#include <cassert>
#include <print>
#include <ranges>
#include <vector>

// XXX(Unavailable): Document usage of ({}) extension. ~~sorry not sorry MSVC~~

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

  Kind             kind;
  std::string_view data;
};

#define TRY(EXPR)                      \
  ({                                   \
      auto _opt_ = (EXPR);             \
      if (!_opt_) return std::nullopt; \
      *_opt_;                          \
  })

[[nodiscard]]
constexpr auto IsWhitespace(char c) noexcept -> bool
{
  return c == ' '
    || c == '\f'
    || c == '\n'
    || c == '\r'
    || c == '\t'
    || c == '\v';
}

constexpr auto SkipWhitespace(std::string_view& str) noexcept
  -> std::optional<std::monostate>
{
  auto view = str | std::views::drop_while(IsWhitespace);
  std::string_view trim {view.begin(), view.end()};
  return (str = trim).empty()
    ? std::nullopt // XXX(Unavailable): diagnostics
    : std::optional{std::monostate{}};
}

// PRECONDITIONS: i <= str.size()
constexpr auto SplitAt(std::string_view& str, std::size_t i) noexcept
  -> std::string_view
{
  assert(i <= str.size());

  auto left = str.substr(0, i);
  str = str.substr(i);
  return {left};
}

// PRECONDITIONS: str is not empty and starts with '"'.
constexpr auto ParseString(std::string_view& str) noexcept
  -> std::optional<std::string_view>
{
  assert(str.starts_with('"'));

  auto pos = str.substr(1).find('"');
  return pos == std::string_view::npos
    ? std::nullopt // XXX(Unavailable): diagnostics
    : std::optional{SplitAt(str, pos + 2)};
}

// PRECONDITIONS: str is not empty and starts with '{'.
constexpr auto ParseObject(std::string_view& str) noexcept
  -> std::optional<std::string_view>
{
  assert(str.starts_with('{'));

  // XXX(Unavailable): better name?
  std::size_t counter = 0;

  std::size_t i = 0;
  do
  {
    if (i >= str.size()) return std::nullopt; // XXX(Unavailable): diagnostics

    // FIXME(Unavailable): Only checking '{' '}' is terrible; semantic parsing
    // would be preferred.

    if      (str[i] == '{') counter++;
    else if (str[i] == '}') counter--;

    if (counter == 0) break;
  }
  while (++i);

  return SplitAt(str, i + 1); // XXX(Unavailable): diagnostics
}

constexpr auto NextKeyValue(std::string_view& str) noexcept
  -> std::optional<std::pair<std::string_view, Value>>
{
  TRY(SkipWhitespace(str)); // XXX(Unavailable): diagnostics

  if (str[0] != '"') return std::nullopt; // XXX(Unavailable): diagnostics

  auto key = TRY(ParseString(str)); // XXX(Unavailable): diagnostics

  TRY(SkipWhitespace(str)); // XXX(Unavailable): diagnostics

  std::string_view value;
  Value::Kind valueKind;
  if (str[0] == '"')
  {
    value = TRY(ParseString(str)); // XXX(Unavailable): diagnostics
    valueKind = Value::Kind::STRING;
  }
  else if (str[0] == '{')
  {
    value = TRY(ParseObject(str)); // XXX(Unavailable): diagnostics
    valueKind = Value::Kind::OBJECT;
  }
  else
  {
    return std::nullopt; // XXX(Unavailable): diagnostics
  }

  return std::optional{std::pair{key, Value{valueKind, value}}};
}

// Visit the contents of a `vdf` file contents.
//
// DOCS(Unavailable): Examples
//
// XXX(Unavailable): `bool strict`
template<std::ranges::input_range R>
constexpr auto VisitVDF(std::string_view vdf, const R& keys) noexcept
  -> std::optional<std::string_view>
{
  if (std::ranges::empty(keys)) return {vdf};

  auto keysBegin = std::ranges::begin(keys);
  const auto keysEnd = std::ranges::end(keys);

  std::string_view curObj = vdf;
  for (;;)
  {
    const auto [key, value] = TRY(NextKeyValue(curObj)); // XXX(unavailable): diagnostics
    const auto keyStripped = key.substr(1, key.size() - 2);

    if (keyStripped == *keysBegin)
    {
      keysBegin = std::next(keysBegin);
      if (keysBegin == keysEnd) return {value.data};

      if (value.kind != Value::Kind::OBJECT) return std::nullopt; // XXX(unavailable): diagnostics
      curObj = value.data.substr(1); // Skip '{'
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
  static_assert(VisitVDF(R"("")",     empty_view) == R"("")");
  static_assert(VisitVDF(R"("a" {})", empty_view) == R"("a" {})");

  // Root
  static_assert(VisitVDF(R"("a" {})", vector_sv{"a"}) == R"({})");

  // Inner
  constexpr auto abclibjav = R"("a" { "b" {} "c" { "lib" "jav" } })";
  static_assert(VisitVDF(abclibjav, vector_sv{"a", "b"}) == R"({})");
  static_assert(VisitVDF(abclibjav, vector_sv{"a", "c"}) == R"({ "lib" "jav" })");

  // Key repeat
  static_assert(VisitVDF(R"("a" { "a" { "a" "a" }})", vector_sv{"a", "a", "a"}) == R"("a")");

  // XXX(Unavailable): Passes for now, shouldn't with `strict = true`.
  static_assert(VisitVDF(R"("a" {{}})"            , empty_view)          == R"("a" {{}})"); // Parser doesn't validate the whole input.
  static_assert(VisitVDF(R"("a" {} "b" {})"       , vector_sv{"b"})      == "{}"); // Top-level doesn't allow multiple keys.
  static_assert(VisitVDF(R"("a" { "b" {}} {{}} })", vector_sv{"a", "b"}) == "{}"); // Parser doesn't validate the whole input.
  // XXX(Unavailable): This should probably also fail with `strict = false`.
  static_assert(VisitVDF(R"("a" { "b" {"a"} })"   , vector_sv{"a", "b"}) == R"({"a"})"); // Object parsing doesn't validate contents inside "{}".

  // XXX(Unavailable): Test steam configuration files.

  std::println("All tests has passed!");

  return 0;
}
