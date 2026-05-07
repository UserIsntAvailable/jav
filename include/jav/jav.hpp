#pragma once

#include <cassert>
#include <optional>
#include <ranges>
#include <string_view>

// XXX(Unavailable): Document that this library only implement the V1 (text
// version) of the file format.
// XXX(Unavailable): Document usage of ({}) extension. ~~sorry not sorry MSVC~~

namespace jav
{

namespace _private
{

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
  str.remove_prefix(i);
  return left;
}

// PRECONDITIONS: str is not empty and starts with '"'.
constexpr auto ParseString(std::string_view& str) noexcept
  -> std::optional<std::string_view>
{
  assert(str.starts_with('"'));

  std::size_t pos = 0;
  for (;;)
  {
    pos = str.find('"', pos + 1);
    if (str[pos-1] != '\\') break;
  }

  return pos == std::string_view::npos
    ? std::nullopt // XXX(Unavailable): diagnostics
    : std::optional{SplitAt(str, pos + 1)};
}

// PRECONDITIONS: str is not empty and starts with '{'.
constexpr auto ParseObject(std::string_view& str) noexcept
  -> std::optional<std::string_view>
{
  assert(str.starts_with('{'));

  std::size_t scopes = 0;
  for (auto [i, chr] : str | std::views::enumerate)
  {
    // FIXME(Unavailable): Only checking '{' '}' is terrible; semantic parsing
    // would be preferred.
    if      (chr == '{') scopes++;
    else if (chr == '}') scopes--;

    if (scopes == 0) return SplitAt(str, i + 1);
  }

  return std::nullopt; // XXX(Unavailable): diagnostics
}

constexpr auto NextKeyValue(std::string_view& str) noexcept
  -> std::optional<std::pair<std::string_view, Value>>
{
  TRY(SkipWhitespace(str)); // XXX(Unavailable): diagnostics

  if (str[0] != '"') return std::nullopt; // XXX(Unavailable): diagnostics

  auto key = TRY(ParseString(str)); // XXX(Unavailable): diagnostics

  TRY(SkipWhitespace(str)); // XXX(Unavailable): diagnostics

  if (str[0] == '"')
  {
    return std::pair{
      key,
      Value{Value::Kind::STRING, TRY(ParseString(str))} // XXX(Unavailable): diagnostics
    };
  }
  else if (str[0] == '{')
  {
    return std::pair{
      key,
      Value{Value::Kind::OBJECT, TRY(ParseObject(str))} // XXX(Unavailable): diagnostics
    };
  }

  return std::nullopt; // XXX(Unavailable): diagnostics
}

} // namespace _private

// Find the value of a key path inside of `vdf` contents.
//
// DOCS(Unavailable): Examples
template<std::ranges::input_range R>
constexpr auto findKey(std::string_view vdf, const R& keys) noexcept
  -> std::optional<std::string_view>
{
  using namespace _private;

  if (std::ranges::empty(keys)) return vdf;

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
      if (keysBegin == keysEnd) return value.data;

      if (value.kind != Value::Kind::OBJECT) return std::nullopt; // XXX(unavailable): diagnostics
      curObj = value.data.substr(1); // Skip '{'
    }
  }

  std::unreachable();
}

} // namespace jav
