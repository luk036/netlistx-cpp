cmake_minimum_required(VERSION 3.14...3.22)

project(
  NetlistX
  VERSION 1.1.7
  LANGUAGES CXX
)

# ---- Include guards ----

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
      "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

# ---------------------------------------------------------------------------
# Compiler warnings (as errors)
# ---------------------------------------------------------------------------

function(target_enable_warnings TARGET)
  if(MSVC)
    target_compile_options(${TARGET} PRIVATE /W4 /WX /utf-8 /wd4819)
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(${TARGET} PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
endfunction()

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
option(NETLISTX_BUILD_TESTS "Build tests" ON)
option(NETLISTX_BUILD_BENCHMARKS "Build benchmarks" OFF)
option(NETLISTX_BUILD_STANDALONE "Build standalone executable" ON)
option(NETLISTX_BUILD_DOCS "Build Doxygen documentation" OFF)
option(NETLISTX_ENABLE_CLANG_TIDY "Enable clang-tidy analysis" OFF)
option(NETLISTX_ENABLE_COVERAGE "Enable coverage reporting" OFF)

# ---------------------------------------------------------------------------
# CUDA (optional)
# ---------------------------------------------------------------------------
find_package(CUDAToolkit QUIET)
if(CUDAToolkit_FOUND)
  message(STATUS "CUDA detected: enabling GPU acceleration")
  set(HAS_CUDA ON)
else()
  message(STATUS "CUDA not found: CPU-only mode")
  set(HAS_CUDA OFF)
endif()

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------

# Download CPM.cmake for reproducible dependency management see
# https://github.com/cpm-cmake/CPM.cmake for more info
set(CPM_DOWNLOAD_VERSION 0.38.5)
set(CPM_HASH_SUM "192aa0ccdc57dfe75bd9e4b176bf7fb5692fd2b3e3f7b09c74856fc39572b31c")
set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
  message(STATUS "Downloading CPM.cmake to ${CPM_DOWNLOAD_LOCATION}")
  file(DOWNLOAD
       https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
       ${CPM_DOWNLOAD_LOCATION} EXPECTED_HASH SHA256=${CPM_HASH_SUM}
  )
endif()
include(${CPM_DOWNLOAD_LOCATION})

# PackageProject.cmake will be used to make our target installable
CPMAddPackage("gh:TheLartians/PackageProject.cmake@1.8.0")

# fmt is a transitive dependency of spdlog. Prefer a system-installed fmt when available. On MSVC,
# skip prebuilt system packages (e.g. conda): they are Release-only shared libraries whose
# std::string ABI (_ITERATOR_DEBUG_LEVEL=0) mismatches Debug builds (_ITERATOR_DEBUG_LEVEL=2),
# corrupting std::strings returned by fmt::format. Build fmt from source on MSVC so the ABI always
# matches (same as CI's windows-latest runner).
if(NOT MSVC)
  find_package(fmt CONFIG QUIET)
  if(fmt_FOUND)
    message(STATUS "Found system fmt: ${fmt_DIR}")
    set(CPM_fmt_ADDED YES)
  endif()
endif()

if(NOT fmt_FOUND)
  CPMAddPackage(
    NAME fmt
    GIT_TAG 12.1.0
    GITHUB_REPOSITORY fmtlib/fmt
    OPTIONS "FMT_INSTALL YES" # create an installable target
  )
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# spdlog must be added before XNetwork (which also adds spdlog) to ensure SPDLOG_FMT_EXTERNAL is
# set, avoiding linker conflicts with fmt when local fmt package is a shared library. Prefer a
# system-installed spdlog on non-MSVC platforms (Ubuntu: libspdlog-dev, macOS: brew install spdlog,
# Termux: spdlog). On MSVC, build from source for the same std::string ABI reason as fmt.
if(NOT MSVC)
  find_package(spdlog CONFIG QUIET)
  if(spdlog_FOUND)
    message(STATUS "Found system spdlog: ${spdlog_DIR}")
    set(CPM_spdlog_ADDED YES)
  endif()
endif()

if(NOT spdlog_FOUND)
  CPMAddPackage(
    NAME spdlog
    GIT_TAG v1.17.0
    GITHUB_REPOSITORY gabime/spdlog
    OPTIONS "SPDLOG_INSTALL YES" "SPDLOG_FMT_EXTERNAL YES"
  )
endif()

# Py2Cpp provides the dict/set containers used across the public headers.
CPMAddPackage(
  NAME Py2Cpp
  GIT_TAG v1.6.4
  GITHUB_REPOSITORY luk036/py2cpp
  OPTIONS "PY2CPP_BUILD_TESTS OFF" "PY2CPP_BUILD_STANDALONE OFF"
)

# XNetwork provides the graph types used by the public headers. The MSVC STL1011 error on
# <experimental/coroutine> used by xnetwork via /await is silenced.
if(MSVC)
  add_compile_definitions(_SILENCE_EXPERIMENTAL_COROUTINE_DEPRECATION_WARNINGS)
endif()
CPMAddPackage(
  NAME XNetwork
  GIT_TAG v1.7.7
  GITHUB_REPOSITORY luk036/xnetwork-cpp
  OPTIONS "XNETWORK_BUILD_TESTS OFF" "XNETWORK_BUILD_STANDALONE OFF"
)

# nlohmann_json is used by readwrite.cpp; always fetch the pinned version and install it so the
# installed package's find_dependency(nlohmann_json 3.12.0) is satisfiable.
CPMAddPackage(
  NAME nlohmann_json
  GIT_TAG v3.12.0
  GITHUB_REPOSITORY nlohmann/json
  OPTIONS "JSON_BuildTests OFF" "JSON_Install ON"
)

set(SPECIFIC_LIBS XNetwork::XNetwork Py2Cpp::Py2Cpp Threads::Threads fmt::fmt spdlog::spdlog
                  nlohmann_json::nlohmann_json
)

# ---------------------------------------------------------------------------
# Library target
# ---------------------------------------------------------------------------

# Note: globbing sources is considered bad practice as CMake's generators may not detect new files
# automatically. Keep that in mind when changing files, or explicitly mention them here.
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
     "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
)
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")

add_library(${PROJECT_NAME} ${headers} ${sources})
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20)

# being a cross-platform target, we enforce standards conformance on MSVC
target_compile_options(${PROJECT_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")

# Public headers include <py2cpp/...> and <xnetwork/...>, so these dependencies are public.
target_link_libraries(${PROJECT_NAME} PUBLIC Py2Cpp::Py2Cpp XNetwork::XNetwork Threads::Threads)
target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt spdlog::spdlog nlohmann_json::nlohmann_json)

# Any TU using spdlog must compile with spdlog's own interface definitions (SPDLOG_COMPILED_LIB,
# SPDLOG_FMT_EXTERNAL) or the header-only and compiled registries diverge.
get_target_property(SPDLOG_COMPILE_DEFS spdlog::spdlog INTERFACE_COMPILE_DEFINITIONS)
if(SPDLOG_COMPILE_DEFS)
  target_compile_definitions(${PROJECT_NAME} PRIVATE ${SPDLOG_COMPILE_DEFS})
endif()

# ---- CUDA support ----
# The .cu kernel is compiled by an explicit nvcc invocation (matching xmake) and linked into the
# test binary; the library itself stays CPU-only.
if(HAS_CUDA)
  message(STATUS "NetlistX: GPU acceleration enabled (CUDA)")
endif()

target_include_directories(
  ${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                         $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

target_enable_warnings(${PROJECT_NAME})

# ---- Create an installable target ----
# this allows users to install and find the library via `find_package()`.

# the location where the project's version header will be placed should match the project's regular
# header paths
string(TOLOWER ${PROJECT_NAME}/version.h VERSION_HEADER_LOCATION)

packageProject(
  NAME ${PROJECT_NAME}
  VERSION ${PROJECT_VERSION}
  NAMESPACE ${PROJECT_NAME}
  BINARY_DIR ${PROJECT_BINARY_DIR}
  INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include
  INCLUDE_DESTINATION include/${PROJECT_NAME}-${PROJECT_VERSION}
  VERSION_HEADER "${VERSION_HEADER_LOCATION}"
  COMPATIBILITY SameMajorVersion
  DEPENDENCIES "fmt 12.1.0" "Py2Cpp 1.6.4" "XNetwork 1.7.7" "nlohmann_json 3.12.0"
)

# ---------------------------------------------------------------------------
# Standalone executable
# ---------------------------------------------------------------------------

if(NETLISTX_BUILD_STANDALONE)
  # cxxopts is a header-only CLI parser used by the standalone example. Prefer a consumer- or
  # system-provided cxxopts (e.g. vcpkg/Conan or a CPM dependency); only fall back to CPM when no
  # cxxopts target is available.
  find_package(cxxopts CONFIG QUIET)
  if(NOT cxxopts_FOUND AND NOT TARGET cxxopts::cxxopts)
    CPMAddPackage(
      GITHUB_REPOSITORY jarro2783/cxxopts
      VERSION 3.2.1
      OPTIONS "CXXOPTS_BUILD_EXAMPLES NO" "CXXOPTS_BUILD_TESTS NO" "CXXOPTS_ENABLE_INSTALL NO"
    )
  endif()
  # keep third-party headers out of our -Werror scope
  if(TARGET cxxopts::cxxopts)
    get_target_property(CXXOPTS_INCLUDE_DIRS cxxopts::cxxopts INTERFACE_INCLUDE_DIRECTORIES)
  endif()

  file(GLOB NETLISTX_STANDALONE_SOURCES CONFIGURE_DEPENDS "standalone/source/*.cpp")
  add_executable(NetlistXStandalone ${NETLISTX_STANDALONE_SOURCES})
  set_target_properties(NetlistXStandalone PROPERTIES OUTPUT_NAME "NetlistX" CXX_STANDARD 20)
  target_include_directories(NetlistXStandalone SYSTEM PRIVATE ${CXXOPTS_INCLUDE_DIRS})
  target_link_libraries(
    NetlistXStandalone PRIVATE NetlistX::NetlistX cxxopts::cxxopts ${SPECIFIC_LIBS}
  )
  target_enable_warnings(NetlistXStandalone)
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

enable_testing()

if(NETLISTX_BUILD_TESTS)
  # Try system-installed doctest first (Ubuntu: doctest-dev, macOS: brew install doctest, Termux:
  # doctest)
  find_package(doctest CONFIG QUIET)

  if(NOT doctest_FOUND)
    CPMAddPackage(
      NAME doctest
      GITHUB_REPOSITORY doctest/doctest
      VERSION 2.5.2
      OPTIONS "DOCTEST_NO_INSTALL ON" # keep the installed prefix minimal
    )
  endif()

  # Code formatting targets (format / fix-format) via Format.cmake
  CPMAddPackage("gh:TheLartians/Format.cmake@1.7.3")

  # Property-based testing (tests only)
  CPMAddPackage(
    NAME rapidcheck
    GITHUB_REPOSITORY emil-e/rapidcheck
    GIT_TAG master
    OPTIONS "RC_BUILD_TESTS OFF" "RC_BUILD_EXAMPLES OFF" "RC_INSTALL OFF"
  )

  file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS "test/source/*.cpp")
  add_executable(NetlistXTests ${TEST_SOURCES})
  target_link_libraries(
    NetlistXTests PRIVATE doctest::doctest NetlistX::NetlistX ${SPECIFIC_LIBS} rapidcheck
  )
  target_include_directories(NetlistXTests PRIVATE ${rapidcheck_SOURCE_DIR}/include)
  target_compile_definitions(NetlistXTests PRIVATE RAPIDCHECK_H)
  set_target_properties(NetlistXTests PROPERTIES CXX_STANDARD 20)
  target_enable_warnings(NetlistXTests)

  # ---- CUDA GPU object (compiled by nvcc, linked into test binary) ----
  if(HAS_CUDA)
    set(CUDA_SRC "${CMAKE_CURRENT_SOURCE_DIR}/source/rand_cover_gpu.cu")
    set(CUDA_OBJ "${CMAKE_CURRENT_BINARY_DIR}/rand_cover_gpu.obj")

    set(NVCC_FLAGS
        --compile
        --gpu-architecture=compute_75
        --std=c++20
        --use-local-env
        -ccbin
        "${CMAKE_CXX_COMPILER}"
        -I"${CMAKE_CURRENT_SOURCE_DIR}/include"
        -I"${CUDAToolkit_INCLUDE_DIRS}"
        -DHAS_CUDA
        -O2
        -o
        "${CUDA_OBJ}"
    )

    if(MSVC)
      list(
        APPEND
        NVCC_FLAGS
        -allow-unsupported-compiler
        -Xcompiler="/EHsc"
        -Xcompiler="/wd4819"
        -Xcompiler="/O2"
        "-Xcompiler=/MD$<$<CONFIG:Debug>:d>"
      )
    endif()

    add_custom_command(
      OUTPUT ${CUDA_OBJ}
      COMMAND ${CUDAToolkit_BIN_DIR}/nvcc ${NVCC_FLAGS} "${CUDA_SRC}"
      DEPENDS ${CUDA_SRC}
      COMMENT "Compiling rand_cover_gpu.cu with nvcc"
      VERBATIM
    )

    target_sources(NetlistXTests PRIVATE ${CUDA_OBJ})
    target_compile_definitions(NetlistXTests PRIVATE HAS_CUDA)
    target_link_libraries(NetlistXTests PRIVATE CUDA::cudart)
  endif()

  # Tests reference testcases via relative paths like ../../testcases/*, so run them from
  # <build>/test where ../../ resolves to the project root.
  file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test")
  add_test(NAME NetlistXAllTests COMMAND NetlistXTests)
  set_tests_properties(
    NetlistXAllTests PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test"
  )
endif()

# ---------------------------------------------------------------------------
# Benchmarks
# ---------------------------------------------------------------------------

if(NETLISTX_BUILD_BENCHMARKS)
  # nanobench is a header-only benchmarking library
  set(NANOBENCH_VERSION 4.3.11)
  set(NANOBENCH_DIR "${CMAKE_BINARY_DIR}/_deps/nanobench")
  set(NANOBENCH_HEADER "${NANOBENCH_DIR}/nanobench.h")

  if(NOT EXISTS "${NANOBENCH_HEADER}")
    file(MAKE_DIRECTORY "${NANOBENCH_DIR}")
    file(
      DOWNLOAD
      "https://raw.githubusercontent.com/martinus/nanobench/v${NANOBENCH_VERSION}/src/include/nanobench.h"
      "${NANOBENCH_HEADER}"
      STATUS download_status
    )
    list(GET download_status 0 status_code)
    if(NOT status_code EQUAL 0)
      message(FATAL_ERROR "Failed to download nanobench (status ${status_code})")
    endif()
  endif()

  add_library(nanobench INTERFACE)
  target_include_directories(nanobench SYSTEM INTERFACE "${NANOBENCH_DIR}")

  file(GLOB NETLISTX_BENCH_SOURCES CONFIGURE_DEPENDS "bench/source/*.cpp")
  foreach(ONE_BENCH_CPP ${NETLISTX_BENCH_SOURCES})
    get_filename_component(ONE_BENCH_EXEC ${ONE_BENCH_CPP} NAME_WE)
    set(TARGET_NAME Bench_${ONE_BENCH_EXEC})

    add_executable(${TARGET_NAME} ${ONE_BENCH_CPP})
    set_target_properties(${TARGET_NAME} PROPERTIES OUTPUT_NAME ${ONE_BENCH_EXEC} CXX_STANDARD 20)
    target_link_libraries(${TARGET_NAME} PRIVATE nanobench NetlistX::NetlistX ${SPECIFIC_LIBS})
    target_enable_warnings(${TARGET_NAME})
    add_test(NAME ${ONE_BENCH_EXEC} COMMAND ${TARGET_NAME})
  endforeach()
endif()

# ---------------------------------------------------------------------------
# clang-tidy
# ---------------------------------------------------------------------------

if(NETLISTX_ENABLE_CLANG_TIDY)
  find_program(CLANG_TIDY clang-tidy)
  if(CLANG_TIDY)
    add_custom_target(
      clang-tidy
      COMMAND ${CLANG_TIDY} --extra-arg=-std=c++20 --extra-arg=-I${CMAKE_CURRENT_SOURCE_DIR}/include
              ${CMAKE_CURRENT_SOURCE_DIR}/include/netlistx/*.hpp
      COMMENT "Running clang-tidy static analysis"
    )
  else()
    message(WARNING "clang-tidy not found - analysis target unavailable")
  endif()
endif()

# ---------------------------------------------------------------------------
# Doxygen documentation
# ---------------------------------------------------------------------------

if(NETLISTX_BUILD_DOCS)
  find_package(Doxygen QUIET)
  if(DOXYGEN_FOUND)
    set(DOXYGEN_PROJECT_NAME ${PROJECT_NAME})
    set(DOXYGEN_PROJECT_VERSION ${PROJECT_VERSION})
    set(DOXYGEN_PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
    set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/doxygen")

    configure_file(documentation/Doxyfile ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)

    add_custom_target(
      GenerateDocs
      COMMAND ${DOXYGEN_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
      COMMAND echo "Docs written to: ${DOXYGEN_OUTPUT_DIRECTORY}/html"
      COMMENT "Generating Doxygen HTML documentation in ${DOXYGEN_OUTPUT_DIRECTORY}/html"
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    )
  else()
    add_custom_target(
      GenerateDocs COMMAND ${CMAKE_COMMAND} -E echo
                           "Doxygen not found - skipping documentation generation"
    )
  endif()
endif()

# ---------------------------------------------------------------------------
# Coverage report target (GCC/Clang only - requires gcovr)
# ---------------------------------------------------------------------------

if(NETLISTX_ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
  foreach(COVERAGE_TARGET NetlistX NetlistXTests NetlistXStandalone)
    if(TARGET ${COVERAGE_TARGET})
      target_compile_options(${COVERAGE_TARGET} PRIVATE -O0 -g -fprofile-arcs -ftest-coverage)
    endif()
  endforeach()
  foreach(COVERAGE_EXEC NetlistXTests NetlistXStandalone)
    if(TARGET ${COVERAGE_EXEC})
      target_link_options(${COVERAGE_EXEC} PRIVATE -fprofile-arcs -ftest-coverage)
    endif()
  endforeach()

  find_program(GCOVR_PATH gcovr)
  if(GCOVR_PATH)
    set(extra_gcovr_args "")
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
      find_program(LLVM_COV_PATH llvm-cov)
      if(LLVM_COV_PATH)
        list(APPEND extra_gcovr_args --llvm-cov-binary ${LLVM_COV_PATH})
      endif()
    endif()
    add_custom_target(
      coverage
      COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/coverage
      COMMAND
        ${GCOVR_PATH} --root ${PROJECT_SOURCE_DIR} --html-details
        ${CMAKE_BINARY_DIR}/coverage/index.html --exclude test/ --exclude build/ ${extra_gcovr_args}
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      COMMENT "Generating coverage report in ${CMAKE_BINARY_DIR}/coverage/index.html"
    )
  else()
    message(
      WARNING "gcovr not found - coverage target unavailable. Install with: pip install gcovr"
    )
  endif()
endif()
