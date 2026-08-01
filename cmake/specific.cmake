set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(fmt CONFIG QUIET)
if(fmt_FOUND)
  message(STATUS "Found system fmt: ${fmt_DIR}")
  # Tell CPM that fmt is already handled (CPM checks CPM_PACKAGES list). Write the CACHE
  # variable directly: list(APPEND ...) creates a normal-variable shadow that does not
  # propagate into FetchContent subdirectory scopes.
  if(NOT fmt IN_LIST CPM_PACKAGES)
    set(CPM_PACKAGES "${CPM_PACKAGES};fmt" CACHE INTERNAL "" FORCE)
  endif()
else()
  CPMAddPackage(
    NAME fmt
    GIT_TAG 12.1.0
    GITHUB_REPOSITORY fmtlib/fmt
    OPTIONS "FMT_INSTALL YES" # create an installable target
  )
endif()

# CPMAddPackage("gh:microsoft/GSL@3.1.0")

CPMAddPackage(
  NAME Py2Cpp
  GIT_TAG v1.6.3
  GITHUB_REPOSITORY luk036/py2cpp
  OPTIONS "INSTALL_ONLY ON" # create an installable target
)

# Suppress MSVC STL1011 error on <experimental/coroutine> used by xnetwork via /await
if(MSVC)
  add_compile_definitions(_SILENCE_EXPERIMENTAL_COROUTINE_DEPRECATION_WARNINGS)
endif()

CPMAddPackage(
  NAME XNetwork
  GIT_TAG v1.7.6
  GITHUB_REPOSITORY luk036/xnetwork-cpp
  OPTIONS "INSTALL_ONLY ON" # create an installable target
)

# When fmt is from system, tell spdlog to use it externally to avoid
# its bundled fmt conflicting with the installed fmt::fmt targets.
if(fmt_FOUND)
  set(SPDLOG_FMT_EXTERNAL YES)
endif()
CPMAddPackage(
  NAME spdlog
  GIT_TAG v1.17.0
  GITHUB_REPOSITORY gabime/spdlog
  OPTIONS "SPDLOG_INSTALL YES" "SPDLOG_COMPILED_LIB ON" # create an installable target and compile
                                                        # as library
          "SPDLOG_FMT_EXTERNAL ${SPDLOG_FMT_EXTERNAL}"
)

# CPMAddPackage( NAME nlohmann_json GITHUB_REPOSITORY nlohmann/json GIT_TAG v3.12.0 OPTIONS
# "JSON_BuildTests OFF" "JSON_Install OFF" # we don't need the installable target, just the header )
# set(NLOHMANN_JSON_INSTALL ON CACHE BOOL "" FORCE)
CPMAddPackage("gh:nlohmann/json@3.12.0")

set(SPECIFIC_LIBS XNetwork::XNetwork Py2Cpp::Py2Cpp Threads::Threads fmt::fmt spdlog::spdlog
                  nlohmann_json::nlohmann_json
)
