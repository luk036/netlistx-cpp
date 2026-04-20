# AGENTS.md - Agentic Coding Guidelines for netlistx-cpp

This file provides context and guidelines for AI agents operating in this codebase.

## Project Overview

netlistx-cpp is a C++ library for netlist/graph representation and algorithms. It provides:
- Netlist data structures (Netlist, SimpleNetlist) with module/net/pad nodes
- Graph algorithms (min_vertex_cover, min_maximal_matching)
- File I/O for IBM .netD/.net and JSON formats

## Build Commands

### Standard Build (CMake)

```bash
# Build all targets (recommended for development)
cmake -S all -B build
cmake --build build

# Build test only
cmake -S test -B build/test && cmake --build build/test
```

### Running Tests

```bash
# Run all tests via ctest
CTEST_OUTPUT_ON_FAILURE=1 cmake --build build/test --target test

# Run test executable directly
./build/test/NetlistXTests

# Run a single test case (doctest)
./build/test/NetlistXTests -tc="Test Netlist"
./build/test/NetlistXTests -tc="Test min_vertex_cover dwarf"
```

### Code Formatting

```bash
# Check/Apply formatting
cmake --build build/test --target format      # check
cmake --build build/test --target fix-format # apply
```

Requires: `clang-format==14.0.6`, `cmake-format==0.6.11`, `pyyaml`

### Additional Tools

```bash
# Sanitizers: Address, Memory, Undefined, Thread, Leak
cmake -S all -B build -DUSE_SANITIZER=Address

# Static analyzers: clang-tidy, iwyu, cppcheck
cmake -S all -B build -DUSE_STATIC_ANALYZER=clang-tidy
```

## Code Style Guidelines

### C++ Standard
- **C++17** required
- MSVC: `/permissive-` (standards conformance)

### Formatting (clang-format)
- **BasedOnStyle**: Google, ColumnLimit: 100, IndentWidth: 4
- **BreakBeforeBraces**: Attach, **NamespaceIndentation**: All

### Naming Conventions

| Element | Convention | Example |
|---------|-----------|---------|
| Classes/Structs | PascalCase | `Netlist`, `PartInfo` |
| Functions | snake_case | `writeJSON`, `readNetD` |
| Variables | snake_case | `num_modules`, `module_weight` |
| Member vars | trailing underscore | `num_modules_` |
| Constants | UPPER_SNAKE | `MAX_DEGREE` |
| Enums | PascalCase | `nodes { mod0, mod1 }` |
| Files | snake_case | `netlist.hpp` |

### Import Order (IncludeBlocks: Regroup)
1. Main project header, 2. Local headers, 3. xnetwork, 4. py2cpp, 5. Std lib, 6. External

```cpp
#include <netlistx/netlist.hpp>  // project
#include "other_local.hpp"       // local
#include <xnetwork/...>      // xnetwork
#include <py2cpp/...>      // py2cpp
#include <algorithm>        // std
#include <fmt/...>        // external
```

### Error Handling
- **Preferred**: Return error codes or `std::optional`/`std::expected`
- **Avoid**: Exceptions except for truly exceptional conditions

### Type Usage
- Use `std::size_t` for sizes/counts
- Use `std::uint32_t`, `std::uint8_t` for fixed-width integers
- Prefer `auto` for deduced types, explicit for public APIs

## Project Structure

```
include/netlistx/     # Public headers: netlist.hpp, netlist_algo.hpp, cover.hpp
source/             # Implementation: readwrite.cpp, netlist.cpp, logger.cpp
test/source/        # Tests: test_netlist.cpp, test_netlist_algo.cpp
standalone/        # Example executable
cmake/             # CMake utilities
```

## Testing Framework

- **doctest**: Main test framework
- **RapidCheck**: Property-based testing (optional)

### Writing Tests

```cpp
#include <doctest/doctest.h>

TEST_CASE("Test description") {
    auto result = myFunction();
    CHECK(result == expected);
}
```

### Running Specific Tests

```bash
./build/test/NetlistXTests -tc="Test Netlist"
./build/test/NetlistXTests --list-test-cases
```

## Key Dependencies

| Library | Purpose |
|---------|---------|
| fmt 10.2.1 | String formatting |
| doctest 2.4.9 | Testing |
| xnetwork | Graph data structures |
| py2cpp | Pythonic collections |

## Common Tasks

- **Add header**: `include/netlistx/*.hpp` → `source/*.cpp` if needed
- **Add test**: `test/source/test_*.cpp` → use doctest macros
- **Build**: `cmake --build build`
- **Format**: `cmake --build build --target fix-format`

## Important Notes

- **No exceptions** in normal error handling
- **No `as any`** or type suppression
- **Format before commit** - run `fix-format`
- **Build after changes** - verify build passes
- **Test after changes** - run tests before submitting