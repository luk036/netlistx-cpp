/**
 * @file main.cpp
 * @brief Standalone executable entry point for NetlistX.
 *
 * Provides a CLI tool with --help, --version, --name, and --lang options.
 * Currently serves as a scaffold for demonstration purposes.
 */

#include <netlistx/greeter.h>
#include <netlistx/version.h>

#include <cxxopts.hpp>
#include <iostream>
#include <string>
#include <unordered_map>

auto main(int argc, char** argv) -> int {
    const std::unordered_map<std::string, netlistx::LanguageCode> languages{
        {"en", netlistx::LanguageCode::EN},
        {"de", netlistx::LanguageCode::DE},
        {"es", netlistx::LanguageCode::ES},
        {"fr", netlistx::LanguageCode::FR},
    };

    cxxopts::Options options(*argv, "A program to welcome the world!");

    std::string language;
    std::string name;

    // clang-format off
  options.add_options()
    ("h,help", "Show help")
    ("v,version", "Print the current version number")
    ("n,name", "Name to greet", cxxopts::value(name)->default_value("World"))
    ("l,lang", "Language code to use", cxxopts::value(language)->default_value("en"))
  ;
    // clang-format on

    auto result = options.parse(argc, argv);

    if (result["help"].as<bool>()) {
        std::cout << options.help() << '\n';
        return 0;
    }

    if (result["version"].as<bool>()) {
        std::cout << "NetlistX, version " << NETLISTX_VERSION << '\n';
        return 0;
    }

    auto langIt = languages.find(language);
    if (langIt == languages.end()) {
        std::cerr << "unknown language code: " << language << '\n';
        return 1;
    }

    return 0;
}
