cmake_minimum_required(VERSION 3.14)
project(NetlistXInstalledTest LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find dependencies first - the installed config references fmt, Py2Cpp, XNetwork and nlohmann_json.
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(Py2Cpp REQUIRED)
find_package(XNetwork REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(NetlistX REQUIRED)

add_executable(netlistx_installed_test main.cpp)
target_link_libraries(netlistx_installed_test PRIVATE NetlistX::NetlistX Threads::Threads)

enable_testing()
add_test(NAME netlistx_installed_test COMMAND netlistx_installed_test)
