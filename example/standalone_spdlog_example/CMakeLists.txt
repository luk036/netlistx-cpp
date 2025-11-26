cmake_minimum_required(VERSION 3.14)

project(standalone_spdlog_example)

add_executable(standalone_spdlog_example source/main.cpp)

target_link_libraries(standalone_spdlog_example PRIVATE spdlog::spdlog)

# Optional: Add install rules if you want to install this example
install(TARGETS standalone_spdlog_example DESTINATION bin)
