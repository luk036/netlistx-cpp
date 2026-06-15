cmake_minimum_required(VERSION 3.14)

project(NetlistXTests LANGUAGES CXX)

# ---- Options ----

option(ENABLE_TEST_COVERAGE "Enable test coverage" OFF)
option(TEST_INSTALLED_VERSION "Test the version found by find_package" OFF)

# --- Import tools ----

include(../cmake/tools.cmake)

# ---- Dependencies ----

include(../cmake/CPM.cmake)
include(../cmake/specific.cmake)

CPMAddPackage("gh:doctest/doctest@2.4.9")
CPMAddPackage("gh:TheLartians/Format.cmake@1.7.3")
CPMAddPackage(
  NAME rapidcheck
  GITHUB_REPOSITORY emil-e/rapidcheck
  GIT_TAG master
  OPTIONS "RC_BUILD_TESTS OFF" "RC_BUILD_EXAMPLES OFF" "RC_INSTALL OFF"
)

if(TEST_INSTALLED_VERSION)
  find_package(NetlistX REQUIRED)
else()
  CPMAddPackage(NAME NetlistX SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..)
endif()

# ---- Optional CUDA GPU acceleration ----
find_package(CUDAToolkit QUIET)
if(CUDAToolkit_FOUND)
  message(STATUS "NetlistXTests: GPU acceleration enabled")
  set(CUDA_ENABLED ON)
else()
  set(CUDA_ENABLED OFF)
endif()

# ---- Create binary ----

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)
add_executable(${PROJECT_NAME} ${sources})

# ---- CUDA GPU object (compiled by nvcc, linked into test binary) ----
if(CUDA_ENABLED)
  set(CUDA_SRC "${CMAKE_CURRENT_LIST_DIR}/../source/rand_cover_gpu.cu")
  set(CUDA_OBJ "${CMAKE_CURRENT_BINARY_DIR}/rand_cover_gpu.obj")

  # Flags matching xmake: CUDA 12.3, compute 7.5, C++20, device link
  set(NVCC_FLAGS
      --compile
      --gpu-architecture=compute_75
      --std=c++20
      --use-local-env
      -ccbin
      "${CMAKE_CXX_COMPILER}"
      -I"${CMAKE_CURRENT_LIST_DIR}/../include"
      -I"${CMAKE_CURRENT_LIST_DIR}/../include/netlistx"
      -I"${CUDAToolkit_INCLUDE_DIRS}"
      -DHAS_CUDA
      -O2
      -o
      "${CUDA_OBJ}"
  )

  if(MSVC)
    list(APPEND NVCC_FLAGS -Xcompiler="/EHsc" -Xcompiler="/wd4819" -Xcompiler="/O2"
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

  target_sources(${PROJECT_NAME} PRIVATE ${CUDA_OBJ})
  target_compile_definitions(${PROJECT_NAME} PRIVATE HAS_CUDA)
  target_link_libraries(${PROJECT_NAME} CUDA::cudart)
endif()

target_link_libraries(
  ${PROJECT_NAME} doctest::doctest NetlistX::NetlistX ${SPECIFIC_LIBS} nlohmann_json::nlohmann_json
)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20)

# Add RapidCheck support if available if(rapidcheck_ADDED)
# target_compile_definitions(${PROJECT_NAME} PRIVATE RAPIDCHECK_H) if(TARGET rapidcheck::rapidcheck)
# target_link_libraries(${PROJECT_NAME} rapidcheck::rapidcheck) else()
# target_include_directories(${PROJECT_NAME} PRIVATE ${rapidcheck_SOURCE_DIR}/include) if(TARGET
# rapidcheck) target_link_libraries(${PROJECT_NAME} rapidcheck) endif() endif() endif()

# enable compiler warnings
if(NOT TEST_INSTALLED_VERSION)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_compile_options(
      NetlistX PUBLIC -Wall -Wpedantic -Wextra -Werror -Wno-deprecated-literal-operator
    )
  elseif(MSVC)
    target_compile_options(NetlistX PUBLIC /W4 /wd4819 /wd4996 /wd5051)
    # target_compile_definitions(${PROJECT_NAME} PUBLIC DOCTEST_CONFIG_USE_STD_HEADERS)
  endif()
endif()

# ---- Add NetlistXTests ----

enable_testing()

# Add a single test that runs all tests (both doctest and RapidCheck) Note: doctest auto-discovery
# is disabled to avoid conflicts with RapidCheck
add_test(NAME NetlistXAllTests COMMAND ${PROJECT_NAME})

# ---- code coverage ----

if(ENABLE_TEST_COVERAGE)
  target_compile_options(NetlistX PUBLIC -O0 -g -fprofile-arcs -ftest-coverage)
  target_link_options(NetlistX PUBLIC -fprofile-arcs -ftest-coverage)
endif()
