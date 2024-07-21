set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_package(Boost REQUIRED COMPONENTS container)
if(Boost_FOUND)
  message(STATUS "Found boost: ${Boost_LIBRARIES}")
  # add_library(Boost::boost INTERFACE IMPORTED GLOBAL) target_include_directories(Boost::boost
  # SYSTEM INTERFACE ${Boost_INCLUDE_DIRS})
endif()

CPMAddPackage(
  NAME fmt
  GIT_TAG 10.2.1
  GITHUB_REPOSITORY fmtlib/fmt
  OPTIONS "FMT_INSTALL YES" # create an installable target
)

# CPMAddPackage("gh:microsoft/GSL@3.1.0")

CPMAddPackage(
  NAME Py2Cpp
  GIT_TAG 1.4.9
  GITHUB_REPOSITORY luk036/py2cpp
  OPTIONS "INSTALL_ONLY ON" # create an installable target
)

CPMAddPackage(
  NAME XNetwork
  GIT_TAG 1.6.8
  GITHUB_REPOSITORY luk036/xnetwork-cpp
  OPTIONS "INSTALL_ONLY ON" # create an installable target
)

set(SPECIFIC_LIBS XNetwork::XNetwork Py2Cpp::Py2Cpp Boost::boost Threads::Threads fmt::fmt)
