# JAV (Json At Valve)

A `library` and `cli` to validate and query `.vdf` (Valve Data Files)
files.

_Only the V1 version (text only) of the file format is currently
supported_.

## Building

_Currently this can't be compiled with MSVC, since this package relies
on the `braced-group within expressions` GNU extension_.

`jav` is implemented as single header library. To compile the project
you need at least `C++23`.

```bash
# Build cli
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build

# or, alternatively only build the library.
cmake -B build -DBUILD_CLI=OFF
cmake --build build

# Install to CMAKE_INSTALL_PREFIX (usually /usr)
# Change preferred location with `--prefix`.
cmake --install build
```

## Usage

```bash
# in sample.vdf
# "toplevel" { "hello" "world" }

# Verifies that `sample.vdf` is valid.
jav sample.vdf # prints: "toplevel" { "hello" "world" }

# Query the `toplevel` key.
jav -q toplevel sample.vdf # prints: { "hello" "world" }

# Recursively find the value of a chain of keys.
jav -q toplevel -q hello sample.vdf # prints: "world"
```
