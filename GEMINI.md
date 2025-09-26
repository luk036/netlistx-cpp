# Gemini Project Analysis: netlistx-cpp

## Project Overview

This project is a C++ template designed for modern C++ development. It provides a solid foundation for creating new C++ projects with a focus on best practices, including:

*   **Modern CMake:** Utilizes modern CMake practices for a clean and maintainable build system.
*   **Testing:** Integrated test suite using `ctest`.
*   **CI/CD:** Continuous integration setup with GitHub Actions for macOS, Windows, and Ubuntu.
*   **Code Quality:** Enforces code formatting with `clang-format` and `cmake-format`.
*   **Dependency Management:** Uses `CPM.cmake` for reproducible dependency management.
*   **Documentation:** Automatic documentation generation with Doxygen and GitHub Pages.

The project is structured to separate the core library from executables and tests, promoting a modular and scalable architecture.

## Building and Running

This project supports both CMake and xmake for building and running the code.

### CMake

The `README.md` file provides detailed instructions for building with CMake. Here are the key commands:

**Build all targets (recommended for development):**

```bash
cmake -S all -B build
cmake --build build
```

**Run tests:**

```bash
./build/test/NetlistXTests
```

**Run standalone executable:**

```bash
./build/standalone/NetlistX --help
```

**Format code:**

```bash
cmake --build build --target fix-format
```

**Build documentation:**

```bash
cmake --build build --target GenerateDocs
```

### xmake

The `xmake.lua` file provides instructions for building and running with xmake.

**Build project:**

```bash
xmake
```

**Run tests:**

```bash
xmake run test_netlistx
```

## Development Conventions

*   **Coding Style:** The project uses `clang-format` for C++ code and `cmake-format` for CMake files. Configuration files (`.clang-format` and `.cmake-format`) are present in the root directory.
*   **Testing:** Tests are located in the `test` directory and can be run using `ctest` or by directly executing the test binary.
*   **Continuous Integration:** The `.github/workflows` directory contains several GitHub Actions workflows for building, testing, and deploying the project on different platforms.
*   **Dependencies:** Dependencies are managed using `CPM.cmake`, which is included in the `cmake` directory.
