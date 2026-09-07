#include <cstdint>
#include <fstream>
#include <netlistx/netlist.hpp>
#include <netlistx/readwrite.hpp>
#include <nlohmann/json.hpp>
#include <py2cpp/set.hpp>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>
#include <xnetwork/classes/graph.hpp>

#include "netlist_builder.hpp"
#include "reader.hpp"

using namespace std;

namespace {

    /**
     * @brief Intermediate representation shared by the DOM and SAX Yosys readers
     */
    struct YosysParts {
        std::vector<std::string> cell_names;
        std::vector<std::string> port_names;
        std::set<uint32_t> all_net_ids;
        std::vector<std::pair<uint32_t, uint32_t>> cell_edges;  // (cell index, raw net id)
        std::unordered_map<std::string, std::set<uint32_t>> port_nets;
    };

    /**
     * @brief Build a SimpleNetlist from parsed Yosys parts (shared phase 2)
     *
     * Node numbering:
     *   - Cell nodes (modules):   0 .. C-1
     *   - Port nodes (modules):   C .. C+P-1
     *   - Net nodes:              C+P .. C+P+N-1
     *
     * where C = number of cells, P = number of ports, N = number of nets.
     */
    auto build_netlist(YosysParts parts) -> SimpleNetlist {
        auto num_cells = static_cast<uint32_t>(parts.cell_names.size());
        auto num_ports = static_cast<uint32_t>(parts.port_names.size());

        // Build sorted net list and net_id -> node mapping
        std::vector<uint32_t> nets_list(parts.all_net_ids.begin(), parts.all_net_ids.end());
        auto num_nets = static_cast<uint32_t>(nets_list.size());
        auto net_start = num_cells + num_ports;

        std::unordered_map<uint32_t, uint32_t> net_to_node;
        for (uint32_t i = 0; i < num_nets; ++i) {
            net_to_node[nets_list[i]] = net_start + i;
        }

        auto total_nodes = net_start + num_nets;
        xnetwork::SimpleGraph g(total_nodes);

        // Edges: cells -> nets
        for (const auto& [cid, raw_net_id] : parts.cell_edges) {
            auto it = net_to_node.find(raw_net_id);
            if (it != net_to_node.end()) {
                g.add_edge(cid, it->second);
            }
        }

        // Edges: ports -> nets
        auto port_start = num_cells;
        for (uint32_t i = 0; i < num_ports; ++i) {
            auto port_node = port_start + i;
            for (auto net_id : parts.port_nets[parts.port_names[i]]) {
                auto it = net_to_node.find(net_id);
                if (it != net_to_node.end()) {
                    g.add_edge(port_node, it->second);
                }
            }
        }

        // Module weights: cells=1, ports=0
        auto module_weight = std::vector<unsigned int>(num_cells + num_ports, 0U);
        for (uint32_t i = 0; i < num_cells; ++i) {
            module_weight[i] = 1U;
        }

        // Mark port nodes as fixed
        py::set<uint32_t> module_fixed;
        for (uint32_t i = 0; i < num_ports; ++i) {
            module_fixed.insert(port_start + i);
        }

        return netlistx::detail::make_netlist(std::move(g), num_cells + num_ports, num_nets,
                                              num_ports, std::move(module_weight),
                                              std::move(module_fixed));
    }

}  // namespace

/**
 * Writes a JSON representation of the given SimpleNetlist to the specified file.
 *
 * @param jsonFileName The path to the output JSON file.
 * @param hyprgraph The SimpleNetlist to be written to the JSON file.
 */
void writeJSON(const std::string_view jsonFileName, const SimpleNetlist& hyprgraph) {
    auto json = ofstream{std::string(jsonFileName)};
    if (json.fail()) {
        netlistx::detail::fail("Error: Can't open file " + std::string(jsonFileName) + ".\n");
    }
    json << R"({
 "directed": false,
 "multigraph": false,
 "graph": {
)";

    json << R"( "num_modules": )" << hyprgraph.number_of_modules() << ",\n";
    json << R"( "num_nets": )" << hyprgraph.number_of_nets() << ",\n";
    json << R"( "num_pads": )" << hyprgraph.num_pads << "\n";
    json << " },\n";

    json << R"( "nodes": [)"
         << "\n";
    for (const auto& node : hyprgraph.gr) {
        json << "  { \"id\": " << node << " },\n";
    }
    json << " ],\n";

    json << R"( "links": [)"
         << "\n";
    for (const auto& v : hyprgraph) {
        for (const auto& net : hyprgraph.gr[v]) {
            json << "  {\n";
            json << "   \"source\": " << v << ",\n";
            json << "   \"target\": " << net << "\n";
            json << "  },\n";
        }
    }
    json << " ]\n";

    json << "}\n";
}

/**
 * Reads an IBM .netD/.net format file and returns a SimpleNetlist representation.
 *
 * Delegates to the internal NetDReader strategy.
 *
 * @param netDFileName The path to the input .netD/.net file.
 * @return A SimpleNetlist object representing the design in the input file.
 */
auto readNetD(const std::string_view netDFileName) -> SimpleNetlist {
    return netlistx::detail::NetDReader{}.read(netDFileName);
}

/**
 * Reads an IBM .are format file and populates a SimpleNetlist object with the
 * module weights and other data.
 *
 * @param hyprgraph The SimpleNetlist object to populate with the .are file data.
 * @param areFileName The path to the .are format file to read.
 */
void readAre(SimpleNetlist& hyprgraph, const std::string_view areFileName) {
    auto are = ifstream{std::string(areFileName)};
    if (are.fail()) {
        netlistx::detail::fail(" Could not open " + std::string(areFileName) + "\n");
    }

    using node_t = uint32_t;
    constexpr index_t bufferSize = 100;
    char lineBuffer[bufferSize];

    char ch = 0;
    node_t node = 0;
    unsigned int weight = 0;
    auto numModules = hyprgraph.number_of_modules();
    auto padOffset = numModules - hyprgraph.num_pads - 1;
    auto module_weight = vector<unsigned int>(numModules);

    size_t lineno = 1;
    for (size_t idx = 0; idx < numModules; idx++) {
        if (are.eof()) {
            break;
        }
        are.get(ch);
        while (isspace(ch) != 0) {
            are.get(ch);
        }
        if (ch == '\n') {
            lineno++;
            continue;
        }
        if (ch == 'a') {
            are >> node;
        } else if (ch == 'p') {
            are >> node;
            node += static_cast<node_t>(padOffset);
        } else {
            netlistx::detail::fail("Syntax error in line " + std::to_string(lineno) + ":"
                                       + R"(expect keyword "a" or "p")" + "\n",
                                   0);
        }

        are.get(ch);
        while (isspace(ch) != 0) {
            are.get(ch);
        }
        if (isdigit(ch) != 0) {
            are.putback(ch);
            are >> weight;
            module_weight[node] = weight;
        }
        are.getline(lineBuffer, bufferSize);
        lineno++;
    }

    hyprgraph.module_weight = std::move(module_weight);
}

/**
 * @brief Read a Yosys JSON file and convert it to a SimpleNetlist object.
 *
 * Uses nlohmann/json's DOM-style parser (json::parse). For the SAX
 * streaming alternative, see read_yosys_json_sax().
 *
 * String-valued net IDs (constants like "0", "1") are skipped.
 * I/O port nodes are assigned weight 0 and marked as fixed.
 *
 * @param filename Path to Yosys JSON file
 * @return SimpleNetlist object representing the circuit
 */
auto read_yosys_json(const std::string_view filename) -> SimpleNetlist {
    auto file = netlistx::detail::open_input(filename);

    nlohmann::json data;
    file >> data;

    // Get the first (top) module from the Yosys JSON
    auto& modules = data["modules"];
    auto module_name = modules.begin().key();
    auto& module_data = modules[module_name];

    YosysParts parts;

    // --- Collect cells (module nodes 0 .. C-1) and ports, in file order ---
    for (const auto& item : module_data["cells"].items()) {
        parts.cell_names.push_back(item.key());
    }
    for (const auto& item : module_data["ports"].items()) {
        parts.port_names.push_back(item.key());
    }
    auto num_cells = static_cast<uint32_t>(parts.cell_names.size());
    auto num_ports = static_cast<uint32_t>(parts.port_names.size());

    // --- Collect all unique integer net IDs ---
    // Nets from port bits
    for (const auto& [_, port_info] : module_data["ports"].items()) {
        for (auto& bit : port_info["bits"]) {
            if (bit.is_number_integer()) {
                parts.all_net_ids.insert(bit.get<uint32_t>());
            }
        }
    }

    // Nets from netnames
    if (module_data.contains("netnames")) {
        for (const auto& [_, netinfo] : module_data["netnames"].items()) {
            for (auto& bit : netinfo["bits"]) {
                if (bit.is_number_integer()) {
                    parts.all_net_ids.insert(bit.get<uint32_t>());
                }
            }
        }
    }

    // Nets and edges from cell connections (skip string constants like "0", "1")
    for (uint32_t i = 0; i < num_cells; ++i) {
        auto& cell_info = module_data["cells"][parts.cell_names[i]];
        for (const auto& [port_name, connections] : cell_info["connections"].items()) {
            (void)port_name;
            for (auto& net_id : connections) {
                if (net_id.is_number_integer()) {
                    auto raw = net_id.get<uint32_t>();
                    parts.cell_edges.emplace_back(i, raw);
                    parts.all_net_ids.insert(raw);
                }
            }
        }
    }

    // Port connections
    for (uint32_t i = 0; i < num_ports; ++i) {
        auto& port_info = module_data["ports"][parts.port_names[i]];
        auto& port_nets = parts.port_nets[parts.port_names[i]];
        for (auto& bit : port_info["bits"]) {
            if (bit.is_number_integer()) {
                port_nets.insert(bit.get<uint32_t>());
            }
        }
    }

    return build_netlist(std::move(parts));
}

/**
 * @brief SAX event handler for streaming Yosys JSON parsing.
 *
 * Implements the nlohmann::json SAX interface to parse Yosys netlist
 * JSON as a stream of events. Tracks a path stack to determine the
 * current JSON context and collects cell names, port names, net IDs,
 * and connectivity data incrementally.
 *
 * Only processes the first module in the file (matching read_yosys_json).
 */
struct YosysSaxHandler {
    std::vector<std::string> path_stack;  ///< current path through the JSON tree
    std::string key_stack;                ///< most recent key

    // Collected data
    std::vector<std::string> cell_names;
    std::unordered_map<std::string, uint32_t> cell_idx;
    std::set<uint32_t> all_net_ids;
    std::vector<std::string> port_names;
    std::unordered_map<std::string, std::set<uint32_t>> port_nets;
    std::vector<std::pair<uint32_t, uint32_t>> cell_edges;

    // State tracking
    std::string current_cell;
    std::string current_port;
    std::string first_module;
    bool found_first_module = false;
    bool in_array = false;

    /// Build dot-separated path string from the path stack.
    [[nodiscard]] auto path() const -> std::string {
        std::string result;
        for (const auto& seg : path_stack) {
            if (!result.empty()) {
                result += '.';
            }
            result += seg;
        }
        return result;
    }

    // ── SAX callbacks ──────────────────────────────────────────────

    bool null() { return true; }
    bool boolean(bool /*val*/) { return true; }
    bool binary(nlohmann::json::binary_t& /*val*/) { return true; }

    bool number_integer(std::int64_t val) {
        if (val < 0) {
            return true;  // negative net IDs (e.g. -1 = VCC) are skipped
        }
        auto uval = static_cast<uint32_t>(val);
        auto p = path();

        if (p.find(".connections.") != std::string::npos && in_array) {
            auto it = cell_idx.find(current_cell);
            if (it != cell_idx.end()) {
                cell_edges.emplace_back(it->second, uval);
            }
            all_net_ids.insert(uval);
        } else if (p.find(".ports.") != std::string::npos && p.find(".bits") != std::string::npos
                   && in_array) {
            all_net_ids.insert(uval);
            port_nets[current_port].insert(uval);
        } else if (p.find(".netnames.") != std::string::npos && p.find(".bits") != std::string::npos
                   && in_array) {
            all_net_ids.insert(uval);
        }
        return true;
    }

    bool number_unsigned(std::uint64_t val) {
        return number_integer(static_cast<std::int64_t>(val));
    }

    bool number_float(double /*val*/, const std::string& /*s*/) { return true; }

    bool string(std::string& /*val*/) { return true; }

    bool start_object(std::size_t /*elements*/) {
        if (!key_stack.empty()) {
            path_stack.push_back(key_stack);

            // Detect module boundary: entering a module object inside "modules"
            if (path_stack.size() == 2 && path_stack[0] == "modules") {
                if (!found_first_module) {
                    first_module = key_stack;
                    found_first_module = true;
                } else if (key_stack != first_module) {
                    return false;  // stop — we only want the first module
                }
            }
        }
        return true;
    }

    bool end_object() {
        if (!path_stack.empty()) {
            path_stack.pop_back();
        }
        // Detect exit from the first module — stop parsing
        if (found_first_module && path_stack.size() == 1 && path_stack[0] == "modules") {
            return false;
        }
        return true;
    }

    bool start_array(std::size_t /*elements*/) {
        in_array = true;
        if (!key_stack.empty()) {
            path_stack.push_back(key_stack);
        }
        return true;
    }

    bool end_array() {
        in_array = false;
        if (!path_stack.empty()) {
            path_stack.pop_back();
        }
        return true;
    }

    bool key(std::string& val) {
        key_stack = val;
        auto p = path();

        // Track current cell when entering a key under cells
        if (p == "modules." + first_module + ".cells") {
            current_cell = val;
            if (cell_idx.find(current_cell) == cell_idx.end()) {
                cell_idx[current_cell] = static_cast<uint32_t>(cell_names.size());
                cell_names.push_back(current_cell);
            }
        }
        // Track current port when entering a key under ports
        if (p == "modules." + first_module + ".ports") {
            current_port = val;
            if (port_nets.find(current_port) == port_nets.end()) {
                port_nets[current_port] = {};
                port_names.push_back(current_port);
            }
        }
        return true;
    }

    bool parse_error(std::size_t /*position*/, const std::string& /*last_token*/,
                     const nlohmann::detail::exception& /*ex*/) {
        return false;
    }
};

/**
 * @brief Read a Yosys JSON file using SAX-style streaming parsing.
 *
 * Uses nlohmann/json's built-in SAX interface to process the file as
 * a stream of parser events without building the full JSON DOM in memory.
 *
 * Only the first module in the file is processed.
 *
 * @param filename Path to Yosys JSON netlist file
 * @return SimpleNetlist object representing the circuit
 */
auto read_yosys_json_sax(const std::string_view filename) -> SimpleNetlist {
    auto file = netlistx::detail::open_input(filename);

    YosysSaxHandler handler;
    auto ok = nlohmann::json::sax_parse(file, &handler);
    // SAX handler returns false to stop after the first module — that's expected.
    (void)ok;

    YosysParts parts;
    parts.cell_names = std::move(handler.cell_names);
    parts.port_names = std::move(handler.port_names);
    parts.all_net_ids = std::move(handler.all_net_ids);
    parts.cell_edges = std::move(handler.cell_edges);
    parts.port_nets = std::move(handler.port_nets);

    return build_netlist(std::move(parts));
}

/**
 * @brief Detect the input file format from its extension.
 */
auto detect_input_format(const string& filename) -> InputFormat {
    auto n = filename.size();
    if (n >= 4 && filename.substr(n - 4) == ".net") {
        return InputFormat::netD;
    }
    if (n >= 4 && filename.substr(n - 4) == ".hgr") {
        return InputFormat::hmetis;
    }
    if (n >= 5 && filename.substr(n - 5) == ".json") {
        return InputFormat::json;
    }
    if (n >= 6 && filename.substr(n - 6) == ".graph") {
        return InputFormat::hmetis;
    }
    if (n >= 7 && filename.substr(n - 7) == ".dimacs") {
        return InputFormat::dimacs;
    }
    return InputFormat::auto_detect;
}

/**
 * @brief Parse hMetis format: header line (num_nets num_vertices [fmt]) then one line per net.
 */
auto read_hmetis_format(const string& filename) -> SimpleNetlist {
    return netlistx::detail::HmetisReader{}.read(filename);
}

/**
 * @brief Parse JSON format: graph metadata + links array with source/target pairs.
 */
auto read_json_format(const string& filename) -> SimpleNetlist {
    return netlistx::detail::JsonReader{}.read(filename);
}

/**
 * @brief Parse DIMACS format: comment lines (c), problem line (p hypre V E), edge lines (e).
 */
auto read_dimacs_format(const string& filename) -> SimpleNetlist {
    return netlistx::detail::DimacsReader{}.read(filename);
}

/**
 * @brief Parse IBM .netD format: header line, then pin records (a=module, p=pad, s=new net).
 */
auto read_netD_format(const string& filename) -> SimpleNetlist {
    return netlistx::detail::NetDReader{}.read(filename);
}

/**
 * @brief Dispatch to format-specific reader based on detected or explicit format.
 */
auto read_hypergraph(const string& filename, InputFormat format) -> SimpleNetlist {
    auto actual_format = format;
    if (format == InputFormat::auto_detect) {
        actual_format = detect_input_format(filename);
    }
    return netlistx::detail::make_reader(actual_format)->read(filename);
}

/**
 * @brief Write partition in hMetis format: one integer per line.
 */
void write_hmetis_partition(const vector<uint8_t>& part, ostream& os) {
    for (const auto p : part) {
        os << static_cast<int>(p) << "\n";
    }
}

/**
 * @brief Write partition as a JSON array, e.g. [0, 1, 0, 1, ...].
 */
void write_json_partition(const vector<uint8_t>& part, ostream& os) {
    os << "[";
    for (size_t i = 0; i < part.size(); ++i) {
        os << static_cast<int>(part[i]);
        if (i < part.size() - 1) {
            os << ", ";
        }
    }
    os << "]\n";
}

/**
 * @brief Dispatch partition output to the appropriate format writer.
 */
void write_partition(const vector<uint8_t>& part, ostream& os, OutputFormat format) {
    if (format == OutputFormat::json) {
        write_json_partition(part, os);
    } else {
        write_hmetis_partition(part, os);
    }
}
