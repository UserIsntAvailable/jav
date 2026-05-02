#include <cassert>
#include <print>
#include <ranges>
#include <vector>

// XXX(Unavailable): constexpr all things.
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

  // If `kind` is `STRING`, this points to the exact substring of the value,
  // (e.g "value"), however, if `kind` is `OBJECT`, then the start of value
  // points to '{', but it doesn't end at the matching '}'.
  std::string_view data;
  Kind             kind;
};

#define TRY(EXPR)                      \
  ({                                   \
      auto _opt_ = (EXPR);             \
      if (!_opt_) return std::nullopt; \
      *_opt_;                          \
  })

auto IsWhitespace(char c) -> bool
{
  return c == ' '
    || c == '\f'
    || c == '\n'
    || c == '\r'
    || c == '\t'
    || c == '\v';
}

auto SkipWhitespace(std::string_view& str) -> std::optional<std::monostate>
{
  auto view = str | std::views::drop_while(IsWhitespace);
  std::string_view trim {view.begin(), view.end()};
  return (str = trim).empty()
    ? std::nullopt // XXX(Unavailable): diagnostics
    : std::optional{std::monostate{}};
}

auto SplitAt(std::string_view& str, std::size_t i)
  -> std::optional<std::string_view>
{
  if (i > str.size()) return std::nullopt; // XXX(Unavailable): diagnostics

  auto left = str.substr(0, i);
  str = str.substr(i);
  return {left};
}

auto ParseString(std::string_view& str) -> std::optional<std::string_view>
{
  if (str.empty()) return std::nullopt; // XXX(Unavailable): diagnostics
  if (str[0] != '"') return std::nullopt; // XXX(Unavailable): diagnostics

  auto pos = str.substr(1).find('"');
  return pos == std::string_view::npos
    ? std::nullopt // XXX(Unavailable): diagnostics
    : SplitAt(str, pos + 2); // XXX(Unavailable): diagnostics
}

auto ParseObject(std::string_view& str) -> std::optional<std::string_view>
{
  if (str.empty()) return std::nullopt; // XXX(Unavailable): diagnostics
  if (str[0] != '{') return std::nullopt; // XXX(Unavailable): diagnostics

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

auto NextKeyValue(std::string_view& str)
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
    value = str;
    valueKind = Value::Kind::OBJECT;
  }
  else
  {
    return std::nullopt; // XXX(Unavailable): diagnostics
  }

  return std::optional{std::pair{key, Value{value, valueKind}}};
}

// Visit the contents of a `vdf` file contents.
//
// DOCS(Unavailable): Examples
//
// XXX(Unavailable): `bool strict`
template<std::ranges::input_range R>
auto VisitVDF(std::string_view vdf, const R& keys)
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
      if (keysBegin == keysEnd)
      {
        // XXX(Unavailable): std::variant?
        if (value.kind == Value::Kind::OBJECT)
        {
          return ParseObject(curObj); // XXX(Unavailable): diagnostics
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

        TRY(ParseObject(curObj)); // XXX(Unavailable): diagnostics
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
