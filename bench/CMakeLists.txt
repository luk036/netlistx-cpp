cmake_minimum_required(VERSION 3.14)

project(NetlistXBench LANGUAGES CXX)

include(../cmake/CPM.cmake)
include(../cmake/specific.cmake)

CPMAddPackage(NAME NetlistX SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..)

file(GLOB sources CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)
add_executable(${PROJECT_NAME} ${sources})

target_link_libraries(
  ${PROJECT_NAME} NetlistX::NetlistX ${SPECIFIC_LIBS} nlohmann_json::nlohmann_json
)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20)

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE "/EHsc" "/utf-8" "/permissive-")
endif()
