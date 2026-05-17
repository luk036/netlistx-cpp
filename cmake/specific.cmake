set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

CPMAddPackage(
  NAME fmt
  GIT_TAG 12.1.0
  GITHUB_REPOSITORY fmtlib/fmt
  OPTIONS "FMT_INSTALL YES" # create an installable target
)

# CPMAddPackage("gh:microsoft/GSL@3.1.0")

CPMAddPackage(
  NAME Py2Cpp
  GIT_TAG 1.6.0
  GITHUB_REPOSITORY luk036/py2cpp
  OPTIONS "INSTALL_ONLY ON" # create an installable target
)

CPMAddPackage(
  NAME XNetwork
  GIT_TAG 1.7.2
  GITHUB_REPOSITORY luk036/xnetwork-cpp
  OPTIONS "INSTALL_ONLY ON" # create an installable target
)

CPMAddPackage(
  NAME spdlog
  GIT_TAG v1.17.0
  GITHUB_REPOSITORY gabime/spdlog
  OPTIONS "SPDLOG_INSTALL YES" "SPDLOG_COMPILED_LIB ON" # create an installable target and compile
                                                        # as library
)

# CPMAddPackage(
#   NAME nlohmann_json
#   GITHUB_REPOSITORY nlohmann/json
#   GIT_TAG v3.12.0
#   OPTIONS "JSON_BuildTests OFF" "JSON_Install OFF" # we don't need the installable target, just the header
# )
# set(NLOHMANN_JSON_INSTALL ON CACHE BOOL "" FORCE)
CPMAddPackage("gh:nlohmann/json@3.12.0")

set(SPECIFIC_LIBS XNetwork::XNetwork Py2Cpp::Py2Cpp Threads::Threads fmt::fmt spdlog::spdlog nlohmann_json::nlohmann_json)
