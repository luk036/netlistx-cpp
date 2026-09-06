#include <netlistx/version.h>

#include <cxxopts.hpp>
#include <iostream>
#include <netlistx/readwrite.hpp>
#include <string>

auto main(int argc, char** argv) -> int {
    cxxopts::Options options("NetlistX", "Netlist hypergraph reader demo");
    options.add_options()("h,help", "Print usage")("v,version", "Print version")(
        "f,file", "Netlist file to read",
        cxxopts::value<std::string>()->default_value("testcases/dwarf1.netD"));

    const auto result = options.parse(argc, argv);
    if (result.count("help") > 0) {
        std::cout << options.help() << '\n';
        return 0;
    }
    if (result.count("version") > 0) {
        std::cout << "NetlistX, version " << NETLISTX_VERSION << '\n';
        return 0;
    }

    const auto filename = result["file"].as<std::string>();
    const auto hyprgraph = read_hypergraph(filename);
    std::cout << "NetlistX: " << filename << " -> " << hyprgraph.number_of_modules() << " modules, "
              << hyprgraph.number_of_nets() << " nets\n";

    return 0;
}
