cmake_minimum_required(VERSION 3.14...3.22)

project(NetlistXTests LANGUAGES CXX)

# ---- Options ----

option(ENABLE_TEST_COVERAGE "Enable test coverage" OFF)
option(TEST_INSTALLED_VERSION "Test the version found by find_package" OFF)

# --- Import tools ----

include(../cmake/tools.cmake)

# ---- Dependencies ----

include(../cmake/CPM.cmake)
include(../cmake/specific.cmake)

#CPMAddPackage(
#  NAME doctest
#  GIT_TAG v2.6.0
#  GITHUB_REPOSITORY doctest/doctest
#  CPM_ARGS "CMAKE_POLICY_VERSION_MINIMUM 3.5"
#)
CPMAddPackage("gh:TheLartians/Format.cmake@1.7.3")

if(TEST_INSTALLED_VERSION)
  find_package(NetlistX REQUIRED)
else()
  CPMAddPackage(NAME NetlistX SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..)
endif()

# ---- Create binary ----

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)
add_executable(${PROJECT_NAME} ${sources})
target_link_libraries(${PROJECT_NAME} #doctest::doctest
 NetlistX::NetlistX ${SPECIFIC_LIBS})
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17)

# enable compiler warnings
if(NOT TEST_INSTALLED_VERSION)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(NetlistX PUBLIC -Wall -Wpedantic -Wextra -Werror)
  elseif(MSVC)
    target_compile_options(NetlistX PUBLIC /W4 /WX /wd4819)
    target_compile_definitions(${PROJECT_NAME} PUBLIC DOCTEST_CONFIG_USE_STD_HEADERS)
  endif()
endif()

# ---- Add NetlistXTests ----

enable_testing()

# Note: doctest and similar testing frameworks can automatically configure CMake tests. For other
# testing frameworks add the tests target instead: add_test(NAME ${PROJECT_NAME} COMMAND
# ${PROJECT_NAME})

#include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
#doctest_discover_tests(${PROJECT_NAME})

# ---- code coverage ----

if(ENABLE_TEST_COVERAGE)
  target_compile_options(NetlistX PUBLIC -O0 -g -fprofile-arcs -ftest-coverage)
  target_link_options(NetlistX PUBLIC -fprofile-arcs -ftest-coverage)
endif()
