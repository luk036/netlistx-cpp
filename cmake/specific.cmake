set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

CPMAddPackage(
  NAME fmt
  GIT_TAG 10.2.1
  GITHUB_REPOSITORY fmtlib/fmt
  OPTIONS "FMT_INSTALL YES" # create an installable target
)

# CPMAddPackage("gh:microsoft/GSL@3.1.0")

CPMAddPackage(
  NAME Py2Cpp
  GIT_TAG 1.5
  GITHUB_REPOSITORY luk036/py2cpp
  OPTIONS "INSTALL_ONLY ON" # create an installable target
)

CPMAddPackage(
  NAME XNetwork
  GIT_TAG 1.7
  GITHUB_REPOSITORY luk036/xnetwork-cpp
  OPTIONS "INSTALL_ONLY ON" # create an installable target
)

set(SPECIFIC_LIBS XNetwork::XNetwork Py2Cpp::Py2Cpp Threads::Threads fmt::fmt)
