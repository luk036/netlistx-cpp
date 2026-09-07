#include "reader.hpp"

#include <cctype>
#include <cstdlib>
#include <iostream>
#include <netlistx/netlist.hpp>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <xnetwork/classes/graph.hpp>

#include "netlist_builder.hpp"

namespace netlistx::detail {

    void fail(const std::string& msg, const int code) {
        std::cerr << msg;
        std::exit(code);
    }

    auto open_input(const std::string_view filename) -> std::ifstream {
        auto file = std::ifstream{std::string(filename)};
        if (file.fail()) {
            fail("Error: Can't open file " + std::string(filename) + ".\n");
        }
        return file;
    }

    // ── hMetis ─────────────────────────────────────────────────────────────

    auto HmetisReader::read(const std::string_view filename) const -> SimpleNetlist {
        auto file = open_input(filename);

        uint32_t num_nets = 0;
        uint32_t num_vertices = 0;
        uint32_t fmt = 0;
        file >> num_nets >> num_vertices;
        if (file.fail()) {
            fail("Error: Invalid hMetis format in file " + std::string(filename) + ".\n");
        }
        file >> fmt;

        const auto num_modules = num_vertices;
        const auto total_vertices = num_modules + num_nets;
        auto g = graph_t(total_vertices);

        std::string line;
        std::getline(file, line);

        uint32_t net_idx = 0;
        for (; net_idx < num_nets && std::getline(file, line); ++net_idx) {
            if (line.empty() || line[0] == 'c') {
                --net_idx;
                continue;
            }
            std::istringstream iss(line);
            uint32_t v = 0;
            while (iss >> v) {
                if (v < num_modules) {
                    g.add_edge(v, num_modules + net_idx);
                }
            }
        }

        return SimpleNetlist{std::move(g), num_modules, num_nets};
    }

    // ── JSON ───────────────────────────────────────────────────────────────

    auto JsonReader::read(const std::string_view filename) const -> SimpleNetlist {
        auto file = open_input(filename);

        nlohmann::json data;
        file >> data;

        auto& graph = data["graph"];
        const auto num_modules = graph["num_modules"].get<uint32_t>();
        const auto num_nets = graph["num_nets"].get<uint32_t>();
        const auto num_pads = graph["num_pads"].get<uint32_t>();
        const auto total_nodes = num_modules + num_nets;

        xnetwork::SimpleGraph g(total_nodes);
        for (const auto& link : data["links"]) {
            const auto source = link["source"].get<uint32_t>();
            const auto target = link["target"].get<uint32_t>();
            g.add_edge(source, target);
        }

        return make_netlist(std::move(g), num_modules, num_nets, num_pads);
    }

    // ── DIMACS ─────────────────────────────────────────────────────────────

    auto DimacsReader::read(const std::string_view filename) const -> SimpleNetlist {
        auto file = open_input(filename);

        uint32_t num_vertices = 0;
        uint32_t num_nets = 0;
        std::string line;

        while (std::getline(file, line)) {
            if (line.empty()) continue;

            if (line[0] == 'c') {
                continue;
            }

            if (line[0] == 'p') {
                std::istringstream iss(line);
                std::string p;
                std::string hypre;
                iss >> p >> hypre >> num_vertices >> num_nets;
                continue;
            }

            if (line[0] == 'e') {
                continue;
            }
        }

        auto g = graph_t(num_vertices + num_nets);
        return SimpleNetlist{g, num_vertices, num_nets};
    }

    // ── IBM .netD ──────────────────────────────────────────────────────────

    auto NetDReader::read(const std::string_view filename) const -> SimpleNetlist {
        auto netD = open_input(filename);

        using node_t = uint32_t;

        char tmp = 0;
        uint32_t numPins = 0;
        uint32_t numNets = 0;
        uint32_t numModules = 0;
        index_t padOffset = 0;

        netD >> tmp;  // eat 1st 0
        netD >> numPins >> numNets >> numModules >> padOffset;

        const auto num_vertices = numModules + numNets;
        auto g = graph_t(num_vertices);

        constexpr index_t bufferSize = 100;
        char lineBuffer[bufferSize];
        netD.getline(lineBuffer, bufferSize);

        node_t node = 0;
        index_t edgeIdx = numModules - 1;
        char ch = 0;
        uint32_t idx = 0;
        for (; idx < numPins; ++idx) {
            if (netD.eof()) {
                std::cerr << "Warning: Unexpected end of file.\n";
                break;
            }
            netD.get(ch);
            while (std::isspace(ch) != 0) {
                netD.get(ch);
            }
            if (ch == '\n') {
                continue;
            }
            if (ch == 'a') {
                netD >> node;
            } else if (ch == 'p') {
                netD >> node;
                node += padOffset;
            }
            netD.get(ch);
            while (std::isspace(ch) != 0) {
                netD.get(ch);
            }
            if (ch == 's') {
                ++edgeIdx;
            }

            g.add_edge(node, edgeIdx);

            netD.get(ch);
            while (std::isspace(ch) != 0 && ch != '\n') {
                netD.get(ch);
            }
            if (ch != '\n') {
                netD.getline(lineBuffer, bufferSize);
            }
        }

        edgeIdx -= numModules - 1;
        if (edgeIdx < numNets) {
            std::cerr << "Warning: number of nets is not " << numNets << ".\n";
            numNets = edgeIdx;
        } else if (edgeIdx > numNets) {
            fail("Error: number of nets is not " + std::to_string(numNets) + ".\n");
        }
        if (idx < numPins) {
            fail("Error: number of pins is not " + std::to_string(numPins) + ".\n");
        }

        return make_netlist(std::move(g), numModules, numNets, numModules - padOffset - 1);
    }

    // ── Factory ────────────────────────────────────────────────────────────

    auto make_reader(const InputFormat format) -> std::unique_ptr<HypergraphReader> {
        switch (format) {
            case InputFormat::hmetis:
                return std::make_unique<HmetisReader>();
            case InputFormat::json:
                return std::make_unique<JsonReader>();
            case InputFormat::dimacs:
                return std::make_unique<DimacsReader>();
            case InputFormat::netD:
            case InputFormat::auto_detect:
                return std::make_unique<NetDReader>();
        }
        fail("Error: Unknown input format.\n");  // unreachable
    }

}  // namespace netlistx::detail
