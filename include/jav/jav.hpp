#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <format>
#include <iterator>
#include <optional>
#include <ranges>
#include <string_view>
#include <utility>
#include <variant>

// XXX(Unavailable): Document that this library only implement the V1 (text
// version) of the file format.
// XXX(Unavailable): Document usage of ({}) extension. ~~sorry not sorry MSVC~~

namespace jav
{

namespace _private
{

#define TRY(EXPR)                                  \
  ({                                               \
    auto res = (EXPR);                             \
    if (!res) return std::unexpected(res.error()); \
    *res;                                          \
  })

[[nodiscard]]
constexpr inline auto isSpace(char c) noexcept -> bool
{
  return c == ' '
    || c == '\f'
    || c == '\n'
    || c == '\r'
    || c == '\t'
    || c == '\v';
}

template<std::ranges::bidirectional_range R>
class KeyPath
{
public:
  explicit constexpr KeyPath(const R& range)
    : value(),
      begin(std::ranges::cbegin(range)),
      end(std::ranges::cend(range))
    {
      iter = begin;
    };

  constexpr auto matches(std::string_view key) const noexcept -> bool
  {
    return iter != end && *iter == key.substr(1, key.size() - 2);
  }

  constexpr auto moveNext() noexcept -> bool
  {
    auto next = std::next(iter);
    if (next == end) return true;
    iter = next;
    return false;
  }

  constexpr auto movePrev() noexcept -> void
  {
    if (iter == begin)
    {
      assert(false && "movePrev was called without previous call to moveNext or the previous call of moveNext returned true and movePrev was called regardless");
      std::unreachable();
    }
    iter = std::prev(iter);
  }

  std::string_view value;

  std::ranges::const_iterator_t<R> iter;
  std::ranges::const_iterator_t<R> begin;
  std::ranges::const_iterator_t<R> end;
};

} // namespace _private

struct Token
{
  enum struct Kind
  {
    INVALID = 0b00000000,
    STRING  = 0b00000001,
    OCURLY  = 0b00000010,
    CCURLY  = 0b00000100,
    COMMENT = 0b00001000
  };

  Kind kind;
  // COMMENT: Does not contain ending '\n'.
  std::string_view span;
};

struct Location
{
  std::size_t pos;
  std::size_t row;
  std::size_t col;
};

struct Error
{
  struct Eof {};
  struct ExpectedToken
  {
    // Bitmask of the token kinds that where expected.
    // eg. std::to_underlying(Token::Kind::STRING) | std::to_underlying(Token::Kind::OCURLY)
    std::underlying_type_t<Token::Kind> expected;
    Token actual;
  };

  std::variant<
    Eof,
    ExpectedToken
  > kind;
  Location loc;
};

class Lexer
{
public:
  explicit constexpr Lexer(std::string_view buf)
    : buf(buf),
      loc({0, 1, 1})
    {};

  [[nodiscard]]
  constexpr auto next() noexcept -> std::expected<Token, Error>
  {
    using namespace _private;

    char curChar;
    do
    {
      // XXX(Unavailable): Not constepxr
      // ```
      // curChar = TRY(nextChar());
      // ```
      auto res = nextChar();
      if (!res) return std::unexpected(res.error());
      curChar = *res;
    }
    while (isSpace(curChar));

    const auto curPos = loc.pos - 1;
    if (curChar == '"')
    {
      for (;;)
      {
        if (TRY(nextChar()) == '"' && buf[loc.pos - 2] != '\\')
        {
          return Token {
            Token::Kind::STRING,
            buf.substr(curPos, loc.pos - curPos)
          };
        }
      }
    }
    else if (curChar == '{')
    {
      return Token {
        Token::Kind::OCURLY,
        buf.substr(curPos, 1),
      };
    }
    else if (curChar == '}')
    {
      return Token {
        Token::Kind::CCURLY,
        buf.substr(curPos, 1),
      };
    }
    else if (curChar == '/' && TRY(peekChar()) == '/')
    {
      nextCharUntilOrEOF([](auto ch) { return ch == '\n'; });

      return Token {
        Token::Kind::COMMENT,
        // -2 to not include '\n'
        buf.substr(curPos, loc.pos - curPos - 2),
      };
    }
    else
    {
      nextCharUntilOrEOF(isSpace);

      return Token {
        Token::Kind::INVALID,
        buf.substr(curPos, loc.pos - curPos - 1),
      };
    }

    return std::unexpected(Error { Error::Eof {}, loc });
  }

  [[nodiscard]]
  constexpr auto nextNonComment() noexcept -> std::expected<Token, Error>
  {
    Token token;
    do
    {
      // XXX(Unavailable): Not constepxr
      // ```
      // token = TRY(next());
      // ```
      auto res = next();
      if (!res) return std::unexpected(res.error());
      token = *res;
    }
    while (token.kind == Token::Kind::COMMENT);

    return token;
  }

  [[nodiscard]]
  constexpr auto nextChar() noexcept -> std::expected<char, Error>
  {
    if (loc.pos >= buf.size())
    {
      return std::unexpected(Error {
        Error::Eof {},
        loc
      });
    }

    const auto curChar = buf[loc.pos++];
    if (curChar == '\n')
    {
      loc.col = 0;
      loc.row++;
    }
    else
    {
      loc.col++;
    }

    return curChar;
  }

  [[nodiscard]]
  constexpr auto peekChar() const noexcept -> std::expected<char, Error>
  {
    if (loc.pos >= buf.size())
    {
      return std::unexpected(Error {
        Error::Eof {},
        loc
      });
    }

    return buf[loc.pos];
  }

  template<typename Predicate>
  constexpr auto nextCharUntilOrEOF(Predicate pred) -> void
  {
    char curChar;
    do
    {
      const auto result = nextChar();
      if (!result.has_value())
      {
        const auto error = result.error();
        assert(std::holds_alternative<Error::Eof>(error.kind));
        break;
      }
      curChar = *result;
    }
    while (!pred(curChar));
  }

  std::string_view buf;
  Location loc;
};

namespace _private
{

template<std::ranges::bidirectional_range R>
constexpr auto skipToClosingCurly(
  Lexer& lexer,
  KeyPath<R>& keyPath,
  bool currentKeyMatches,
  std::uint16_t recursionLevel
) noexcept
  -> std::expected<std::monostate, Error>
{
  for (;;)
  {
    char curChar;
    while (isSpace(curChar = TRY(lexer.peekChar())))
    {
      TRY(lexer.nextChar());
    }

    if (curChar == '}')
    {
      TRY(lexer.nextChar()); // Consume '}'
      break;
    }
    else if (curChar == '"')
    {
      TRY(nextKey(lexer, keyPath, currentKeyMatches, recursionLevel));
    }
    else
    {
      const auto curPos = lexer.loc.pos;
      lexer.nextCharUntilOrEOF(isSpace);

      return std::unexpected(Error {
        Error::ExpectedToken {
          std::to_underlying(Token::Kind::STRING)
            | std::to_underlying(Token::Kind::CCURLY),
          Token {
            Token::Kind::INVALID,
            lexer.buf.substr(curPos, lexer.loc.pos - curPos),
          }
        },
        lexer.loc,
      });
    }
  }

  return std::monostate{};
}

// Returns whether the function finished because of a non-fatal EOF error.
template<std::ranges::bidirectional_range R>
constexpr auto nextKey(
  Lexer& lexer,
  KeyPath<R>& keyPath,
  bool parentKeyMatches,
  std::uint16_t recursionLevel = 0
) noexcept
  -> std::expected<bool, Error>
{
  // FIXME(Unavailable): Recursion level cap.

  const auto keyResult = lexer.nextNonComment();
  if (!keyResult.has_value())
  {
    const auto error = keyResult.error();
    if (std::holds_alternative<Error::Eof>(error.kind) && recursionLevel == 0)
    {
      return true;
    }
    return std::unexpected(error);
  }

  const auto key = *keyResult;
  if (key.kind != Token::Kind::STRING)
  {
    return std::unexpected(Error {
      Error::ExpectedToken {
        std::to_underlying(Token::Kind::STRING),
        key
      },
      lexer.loc
    });
  }

  const auto currentKeyMatches = parentKeyMatches
    && keyPath.matches(key.span);
  const auto isLastKeySegment = currentKeyMatches && keyPath.moveNext();

  const auto value = TRY(lexer.nextNonComment());
  std::string_view valueSpan {};

  if (value.kind == Token::Kind::STRING)
  {
    valueSpan = value.span;
  }
  else if (value.kind == Token::Kind::OCURLY)
  {
    const auto oCurlyPos = lexer.loc.pos - 1;

    TRY(skipToClosingCurly(
      lexer,
      keyPath,
      currentKeyMatches,
      recursionLevel + 1));
    valueSpan = lexer.buf.substr(oCurlyPos, lexer.loc.pos - oCurlyPos);

    if (currentKeyMatches && !isLastKeySegment) keyPath.movePrev();
  }
  else
  {
    return std::unexpected(Error {
      Error::ExpectedToken {
        std::to_underlying(Token::Kind::STRING)
          | std::to_underlying(Token::Kind::OCURLY),
        key
      },
      lexer.loc
    });
  }

  if (isLastKeySegment) keyPath.value = valueSpan;

  return false;
}

} // namespace _private

[[nodiscard]]
constexpr auto validate(std::string_view vdf) noexcept
  -> std::expected<std::monostate, Error>
{
  using namespace _private;

  Lexer lexer {vdf};
  KeyPath keyPath {std::ranges::empty_view<std::string_view>{}};

  while (!TRY(nextKey(lexer, keyPath, false))) {}

  return std::monostate{};
}

// XXX(Unavailable): Allow multiple keys to be search at the same time.
// TODO(Unavailable): Constrain `R` to yield objects that could be compared with std::string_view

// Find the value of a key path inside of `vdf` contents.
//
// DOCS(Unavailable): Examples
template<std::ranges::bidirectional_range R>
[[nodiscard]]
constexpr auto findKey(std::string_view vdf, const R& path) noexcept
  -> std::expected<std::optional<std::string_view>, Error>
{
  using namespace _private;

  if (std::ranges::empty(path))
  {
    TRY(validate(vdf));
    return std::optional{vdf};
  }

  Lexer lexer {vdf};
  KeyPath keyPath {path};

  while (!TRY(nextKey(lexer, keyPath, true))) {}

  return keyPath.value.empty()
    ? std::nullopt
    : std::optional{keyPath.value};
}

} // namespace jav

namespace std
{

template<>
struct formatter<jav::Error>
{
  template<class... Ts>
  struct overloads : Ts... { using Ts::operator()...; };

  struct TokenRepr
  {
    jav::Token::Kind kind;
    const char* repr;
  };

  static constexpr std::array tokenReprs
  {
    TokenRepr{jav::Token::Kind::STRING, "\""},
    TokenRepr{jav::Token::Kind::OCURLY, "{"},
    TokenRepr{jav::Token::Kind::CCURLY, "}"},
    TokenRepr{jav::Token::Kind::COMMENT, "//"},
  };

  template<class ParseContext>
  constexpr auto parse(ParseContext& ctx) -> ParseContext::iterator
  {
    // XXX(Unavailable): Error out on unknown format specifiers?
    return ctx.begin();
  }

  template<class FormatContext>
  auto format(const jav::Error& err, FormatContext& ctx) const
    -> FormatContext::iterator
  {
    auto out = std::format_to(
      ctx.out(),
      "{}:{}: ",
      err.loc.row,
      err.loc.col
    );

    const auto visitor = overloads {
      [&](const jav::Error::Eof&)
      {
        return std::format_to(std::move(out), "Unexpected end of file");
      },
      [&](const jav::Error::ExpectedToken& e)
      {
        out = std::format_to(std::move(out), "Expected token ");

        const auto isKnownTokenKind = [&](const TokenRepr& token)
        {
          return (e.expected & std::to_underlying(token.kind)) != 0;
        };

        const auto knownKinds = std::ranges::count_if(
          tokenReprs,
          isKnownTokenKind
        );

        if (knownKinds == 0)
        {
          out = std::format_to(std::move(out), "<unknown kind>");
        }
        else
        {
          auto iter = tokenReprs
            | std::views::filter(isKnownTokenKind)
            | std::views::enumerate;
          for (const auto& [index, token] : iter)
          {
            if (index > 0)
            {
              // XXX(Unavailable): why `consteval` doesn't like this ternary?
              // ```
              // out = std::format_to(
              //   std::move(out),
              //   index == knownKinds - 1 ? " or " : ", "
              // );
              // ```
              out = index == knownKinds - 1
                ? std::format_to(std::move(out), " or ")
                : std::format_to(std::move(out), ", ");
            }

            out = std::format_to(std::move(out), "'{}'", token.repr);
          }
        }

        return std::format_to(std::move(out), ", but found '{}'", e.actual.span);
      }
    };

    return std::visit(visitor, err.kind);
  }
};

} // namespace std
