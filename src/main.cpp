#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <print>
#include <vector>
#include <jav/jav.hpp>

// XXX(Unavailable): Accept stdin

struct Args
{
  explicit Args(int argc, char *argv[])
    : buf{std::span{argv, static_cast<long unsigned int>(argc)}}
  {}

  auto shift() noexcept -> std::optional<std::string_view>
  {
    if (buf.empty()) return std::nullopt;
    const auto current = buf[0];
    buf = buf.subspan(1);
    return current;
  }

private:
  std::span<char*> buf;
};

auto showHelp() -> int
{
  std::print(
    R"(A utility that can validate and query Valve Data Files (VDFs).

Only the V1 version (text only) of the file format is currently supported.

Usage:
  jav [OPTIONS] FILE

Options:
  -q, --query <QUERY>  The key that wants to be queried.
                       Can be used multiple times to query at diferent depths.
  -h, --help           Print help
  -V, --version        Print version
)");

  return EXIT_SUCCESS;
}

auto showVersion() -> int
{
  std::println("{}", CLI_VERSION);
  return EXIT_SUCCESS;
}

auto showMisisingArgument(std::string_view programName, auto opt) -> int
{
  std::println("{}: missing argument for option '{}'", programName, opt);
  return EXIT_FAILURE;
}

// The first optional indicates whether opt matched `shortOpt` or `longOpt`; the
// second optional indicates the value found for `opt`.
auto parseOpt(
  std::string_view opt,
  Args &args,
  std::string_view shortOpt,
  std::string_view longOpt
) noexcept
  -> std::optional<std::optional<std::string_view>>
{
  // Parses:
  // -opt [val] --opt [val]
  // -optval
  // --opt=val

  if (opt == shortOpt || opt == longOpt)
  {
    return args.shift();
  }
  else if (opt.starts_with(shortOpt))
  {
    return opt.substr(shortOpt.size());
  }
  else if (opt.starts_with(longOpt))
  {
    const auto value = opt.substr(longOpt.size());
    if (value[0] != '=') return std::optional<std::string_view>{};
    return value.substr(1);
  }

  return std::nullopt;
}

auto readToString(const std::filesystem::path& path)
  -> std::expected<std::string, std::error_code>
{
  std::ifstream file {path};
  if (!file) return std::unexpected(std::make_error_code(std::errc(errno)));

  return std::expected<std::string, std::error_code> {
    std::in_place,
    std::string {
      std::istreambuf_iterator<char> {file},
      std::istreambuf_iterator<char> {}
    }
  };
}

auto main(int argc, char *argv[]) -> int
{
  if (argc <= 1) return showHelp();

  Args args {argc, argv};
  auto programName = *args.shift();

  std::vector<std::string_view> querySegments {};
  std::optional<std::string_view> filePath {};
  bool wasDoubleDashUsed = false;

  std::optional<std::string_view> optResult = args.shift();
  while (optResult.has_value())
  {
    const std::string_view opt = *optResult;

    if (opt == "-h" || opt == "--help")
    {
      return showHelp();
    }
    else if (opt == "-V" || opt == "--version")
    {
      return showVersion();
    }
    else if (auto query = parseOpt(opt, args, "-q", "--query"); query)
    {
      if (!(*query).has_value()) return showMisisingArgument(programName, opt);
      querySegments.push_back(**query);
    }
    else if (opt == "--")
    {
      wasDoubleDashUsed = true;
      break;
    }
    else if (opt.starts_with("-"))
    {
      std::println("{}: unknown option '{}'", programName, opt);
      return EXIT_FAILURE;
    }
    else
    {
      filePath = opt;
    }

    optResult = args.shift();
  }

  if (wasDoubleDashUsed)
  {
    const auto file = args.shift();
    if (!file.has_value()) return showMisisingArgument(programName, "FILE");
    filePath = *file;
  }
  else if (!filePath.has_value())
  {
    return showMisisingArgument(programName, "FILE");
  }

  // XXX(Unavailable): Most VDFs files are fairly small, so loading the whole
  // file contents into memory is not that big of a deal. Allowing streaming
  // would require `Lexer` changes; might look into it in the future :).
  const auto fileContents = readToString(*filePath);
  if (!fileContents.has_value())
  {
    const auto errorCode = fileContents.error();
    std::println(
      "{}: {}: {} (os error {})",
      programName,
      *filePath,
      errorCode.message(),
      errorCode.value()
    );
    return EXIT_FAILURE;
  }

  const auto value = jav::findKey(*fileContents, querySegments);
  if (!value.has_value())
  {
    std::println("{}: {}:{}", programName, *filePath, value.error());
    return EXIT_FAILURE;
  }
  else if (!(*value).has_value())
  {
    return EXIT_FAILURE; // query was not found.
  }

  // XXX(Unavailable): By default, strip quotes from strings, and format objects.
  // XXX(Unavailable): --raw-output to preserve value span

  std::println("{}", **value);

  return EXIT_SUCCESS;
}
