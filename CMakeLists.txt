cmake_minimum_required(VERSION 3.18)

# ---- Project ----

project(
  NetlistX
  VERSION 1.1.5
  LANGUAGES CXX
)

# ---- CUDA (optional) ---
find_package(CUDAToolkit QUIET)
if(CUDAToolkit_FOUND)
  list(APPEND CMAKE_MESSAGE_CONTEXT "CUDA")
  message(STATUS "CUDA detected — enabling GPU acceleration")
  set(HAS_CUDA ON)
else()
  set(HAS_CUDA OFF)
  message(STATUS "CUDA not found — CPU-only mode")
endif()

# ---- Include guards ----

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(
    FATAL_ERROR
      "In-source builds not allowed. Please make a new directory (called a build directory) and run CMake from there."
  )
endif()

# ---- Add dependencies via CPM ----
# see https://github.com/luk036/CPM.cmake for more info

include(cmake/CPM.cmake)
include(cmake/specific.cmake)

# PackageProject.cmake will be used to make our target installable
CPMAddPackage("gh:TheLartians/PackageProject.cmake@1.8.0")

CPMAddPackage(
  NAME fmt
  GIT_TAG 12.1.0
  GITHUB_REPOSITORY fmtlib/fmt
  OPTIONS "FMT_INSTALL YES" # create an installable target
)

# ---- Add source files ----

# Note: globbing sources is considered bad practice as CMake's generators may not detect new files
# automatically. Keep that in mind when changing files, or explicitly mention them here.
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp")

# ---- Create library ----

# Note: for header-only libraries change all PUBLIC flags to INTERFACE and create an interface
# target: add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME} ${headers} ${sources})
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20)

# being a cross-platform target, we enforce standards conformance on MSVC
target_compile_options(${PROJECT_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/permissive->")

# ---- CUDA source files ----
if(HAS_CUDA)
  file(GLOB cuda_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cu")
  target_sources(${PROJECT_NAME} PRIVATE ${cuda_sources})
  set_source_files_properties(${cuda_sources} PROPERTIES LANGUAGE CUDA)
  set_target_properties(${PROJECT_NAME} PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
  target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_CUDA)
  if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=/wd4819>")
  endif()
  message(STATUS "NetlistX: GPU acceleration enabled (CUDA)")
endif()

# Link dependencies
target_link_libraries(${PROJECT_NAME} PRIVATE ${SPECIFIC_LIBS} fmt::fmt)

# Suppress MSVC warnings in spdlog if(MSVC AND TARGET spdlog) target_compile_options(spdlog PRIVATE
# ) endif()

target_include_directories(
  ${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
                         $<INSTALL_INTERFACE:include/${PROJECT_NAME}-${PROJECT_VERSION}>
)

install(
  TARGETS nlohmann_json
  EXPORT NetlistXTargets # 必须和上面一致
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

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
  DEPENDENCIES "fmt 9.1.0"
)
