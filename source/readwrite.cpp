#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <netlistx/netlist.hpp>
#include <netlistx/readwrite.hpp>
#include <py2cpp/range.hpp>
#include <py2cpp/set.hpp>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <xnetwork/classes/graph.hpp>

// using graph_t =
//     boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS>;
// using node_t = typename boost::graph_traits<graph_t>::vertex_descriptor;
// using edge_t = typename boost::graph_traits<graph_t>::edge_iterator;

#include <nlohmann/json.hpp>  // for json
#include <set>                // for set
#include <unordered_map>      // for unordered_map

using namespace std;

/**
 * Writes a JSON representation of the given SimpleNetlist to the specified file.
 *
 * @param jsonFileName The path to the output JSON file.
 * @param hyprgraph The SimpleNetlist to be written to the JSON file.
 */
void writeJSON(const std::string_view jsonFileName, const SimpleNetlist& hyprgraph) {
    auto json = ofstream{std::string(jsonFileName)};
    if (json.fail()) {
        cerr << "Error: Can't open file " << jsonFileName << ".\n";
        exit(1);
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
 * This function reads an IBM .netD/.net format file and constructs a SimpleNetlist
 * object from the data in the file. The SimpleNetlist contains information about
 * the modules, nets, and pins in the design.
 *
 * @param netDFileName The path to the input .netD/.net file.
 * @return A SimpleNetlist object representing the design in the input file.
 */
auto readNetD(const std::string_view netDFileName) -> SimpleNetlist {
    auto netD = ifstream{std::string(netDFileName)};
    if (netD.fail()) {
        cerr << "Error: Can't open file " << netDFileName << ".\n";
        exit(1);
    }

    using node_t = uint32_t;

    char tmp = 0;
    uint32_t numPins = 0;
    uint32_t numNets = 0;
    uint32_t numModules = 0;
    index_t padOffset = 0;

    netD >> tmp;  // eat 1st 0
    netD >> numPins >> numNets >> numModules >> padOffset;

    // using Edge = pair<int, int>;

    const auto num_vertices = numModules + numNets;
    // const auto R = py::range<node_t>(0, num_vertices);
    auto g = graph_t(num_vertices);

    constexpr index_t bufferSize = 100;
    char lineBuffer[bufferSize];  // Does it work for other compiler?
    netD.getline(lineBuffer, bufferSize);

    node_t node = 0;
    index_t edgeIdx = numModules - 1;
    char ch = 0;
    uint32_t idx = 0;
    for (; idx < numPins; ++idx) {
        if (netD.eof()) {
            cerr << "Warning: Unexpected end of file.\n";
            break;
        }
        netD.get(ch);
        while (isspace(ch) != 0) {
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
        while (isspace(ch) != 0) {
            netD.get(ch);
        }
        if (ch == 's') {
            ++edgeIdx;
        }

        // edge_array[idx] = Edge(node, edgeIdx);
        g.add_edge(node, edgeIdx);

        netD.get(ch);
        while (isspace(ch) != 0 && ch != '\n') {
            netD.get(ch);
        }

        // switch (ch) {
        // case 'O': aPin.setDirection(Pin::OUTPUT); break;
        // case 'I': aPin.setDirection(Pin::INPUT); break;
        // case 'B': aPin.setDirection(Pin::BIDIR); break;
        // }
        if (ch != '\n') {
            netD.getline(lineBuffer, bufferSize);
        }
    }

    edgeIdx -= numModules - 1;
    if (edgeIdx < numNets) {
        cerr << "Warning: number of nets is not " << numNets << ".\n";
        numNets = edgeIdx;
    } else if (edgeIdx > numNets) {
        cerr << "Error: number of nets is not " << numNets << ".\n";
        exit(1);
    }
    if (idx < numPins) {
        cerr << "Error: number of pins is not " << numPins << ".\n";
        exit(1);
    }

    // using IndexMap =
    //     typename boost::property_map<graph_t, boost::vertex_index_t>::type;
    // auto index = boost::get(boost::vertex_index, g);
    // auto gr = py::GraphAdaptor<graph_t>{std::move(g)};
    auto hyprgraph = SimpleNetlist{std::move(g), numModules, numNets};
    hyprgraph.num_pads = numModules - padOffset - 1;
    return hyprgraph;
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
        cerr << " Could not open " << areFileName << '\n';
        exit(1);
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
            cerr << "Syntax error in line " << lineno << ":"
                 << R"(expect keyword "a" or "p")" << '\n';
            exit(0);
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
 * Node numbering:
 *   - Cell nodes (modules):   0 .. C-1
 *   - Port nodes (modules):   C .. C+P-1
 *   - Net nodes:              C+P .. C+P+N-1
 *
 * where C = number of cells, P = number of ports, N = number of nets.
 */
auto read_yosys_json(const std::string_view filename) -> SimpleNetlist {
    auto file = ifstream{std::string(filename)};
    if (file.fail()) {
        cerr << "Error: Can't open file " << filename << ".\n";
        exit(1);
    }

    nlohmann::json data;
    file >> data;

    // Get the first (top) module from the Yosys JSON
    auto& modules = data["modules"];
    auto module_name = modules.begin().key();
    auto& module_data = modules[module_name];

    // --- Collect cells (module nodes 0 .. C-1) ---
    vector<string> cell_names;
    for (const auto& [name, _] : module_data["cells"].items()) {
        cell_names.push_back(name);
    }
    auto num_cells = static_cast<uint32_t>(cell_names.size());

    // --- Collect all unique integer net IDs ---
    set<uint32_t> all_nets_set;

    // Nets from port bits
    for (const auto& [_, port_info] : module_data["ports"].items()) {
        for (auto& bit : port_info["bits"]) {
            if (bit.is_number_integer()) {
                all_nets_set.insert(bit.get<uint32_t>());
            }
        }
    }

    // Nets from netnames
    if (module_data.contains("netnames")) {
        for (const auto& [_, netinfo] : module_data["netnames"].items()) {
            for (auto& bit : netinfo["bits"]) {
                if (bit.is_number_integer()) {
                    all_nets_set.insert(bit.get<uint32_t>());
                }
            }
        }
    }

    // Nets from cell connections (skip string constants like "0", "1")
    for (const auto& [_, cell_info] : module_data["cells"].items()) {
        for (const auto& [port_name, connections] : cell_info["connections"].items()) {
            (void)port_name;
            for (auto& net_id : connections) {
                if (net_id.is_number_integer()) {
                    all_nets_set.insert(net_id.get<uint32_t>());
                }
            }
        }
    }

    // Build sorted net list and net_id -> node mapping
    // Net node IDs start at num_cells + num_ports
    auto num_ports = static_cast<uint32_t>(module_data["ports"].size());
    vector<uint32_t> nets_list(all_nets_set.begin(), all_nets_set.end());
    auto num_nets = static_cast<uint32_t>(nets_list.size());
    auto net_start = num_cells + num_ports;

    unordered_map<uint32_t, uint32_t> net_to_node;
    for (uint32_t i = 0; i < num_nets; ++i) {
        net_to_node[nets_list[i]] = net_start + i;
    }

    // Total graph nodes = cells + ports + nets
    auto total_nodes = net_start + num_nets;

    // --- Build the bipartite graph ---
    xnetwork::SimpleGraph g(total_nodes);

    // Edges: cells -> nets
    for (uint32_t i = 0; i < num_cells; ++i) {
        auto& cell_info = module_data["cells"][cell_names[i]];
        for (const auto& [_, connections] : cell_info["connections"].items()) {
            for (auto& net_id : connections) {
                if (net_id.is_number_integer()) {
                    auto net_node = net_to_node[net_id.get<uint32_t>()];
                    g.add_edge(i, net_node);
                }
            }
        }
    }

    // Edges: ports -> nets
    auto port_start = num_cells;
    uint32_t port_idx = 0;
    for (const auto& [_, port_info] : module_data["ports"].items()) {
        auto port_node = port_start + port_idx;
        for (auto& bit : port_info["bits"]) {
            if (bit.is_number_integer()) {
                auto net_id = bit.get<uint32_t>();
                auto it = net_to_node.find(net_id);
                if (it != net_to_node.end()) {
                    g.add_edge(port_node, it->second);
                }
            }
        }
        ++port_idx;
    }

    // --- Create Netlist ---
    // modules = range(0, num_cells + num_ports), nets = range(net_start, total_nodes)
    auto hyprgraph = SimpleNetlist{std::move(g), num_cells + num_ports, num_nets};
    hyprgraph.num_pads = num_ports;

    // Module weights: cells=1, ports=0
    hyprgraph.module_weight.assign(num_cells + num_ports, 0);
    for (uint32_t i = 0; i < num_cells; ++i) {
        hyprgraph.module_weight[i] = 1;
    }

    // Mark port nodes as fixed
    for (uint32_t i = 0; i < num_ports; ++i) {
        hyprgraph.module_fixed.insert(port_start + i);
    }
    hyprgraph.has_fixed_modules = (num_ports > 0);

    return hyprgraph;
}

// Helper: map file extension to InputFormat based on suffix matching.
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

// Parse hMetis format: header line (num_nets num_vertices [fmt]) then one line per net.
auto read_hmetis_format(const string& filename) -> SimpleNetlist {
    auto file = ifstream{filename};
    if (file.fail()) {
        cerr << "Error: Can't open file " << filename << ".\n";
        exit(1);
    }

    uint32_t num_nets = 0;
    uint32_t num_vertices = 0;
    uint32_t fmt = 0;
    file >> num_nets >> num_vertices;
    if (file.fail()) {
        cerr << "Error: Invalid hMetis format in file " << filename << ".\n";
        exit(1);
    }
    file >> fmt;

    const auto num_modules = num_vertices;
    const auto total_vertices = num_modules + num_nets;
    auto g = graph_t(total_vertices);

    string line;
    getline(file, line);

    uint32_t net_idx = 0;
    for (; net_idx < num_nets && getline(file, line); ++net_idx) {
        if (line.empty() || line[0] == 'c') {
            --net_idx;
            continue;
        }
        istringstream iss(line);
        uint32_t v = 0;
        while (iss >> v) {
            if (v < num_modules) {
                g.add_edge(v, num_modules + net_idx);
            }
        }
    }

    return SimpleNetlist{g, num_modules, num_nets};
}

// Parse JSON format: graph metadata + links array with source/target pairs.
auto read_json_format(const string& filename) -> SimpleNetlist {
    auto file = ifstream{filename};
    if (file.fail()) {
        cerr << "Error: Can't open file " << filename << ".\n";
        exit(1);
    }

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

    auto hyprgraph = SimpleNetlist{std::move(g), num_modules, num_nets};
    hyprgraph.num_pads = num_pads;
    return hyprgraph;
}

// Parse DIMACS format: comment lines (c), problem line (p hypre V E), edge lines (e).
// Currently only reads the header and creates an empty graph with the declared dimensions.
auto read_dimacs_format(const string& filename) -> SimpleNetlist {
    auto file = ifstream{filename};
    if (file.fail()) {
        cerr << "Error: Can't open file " << filename << ".\n";
        exit(1);
    }

    uint32_t num_vertices = 0;
    uint32_t num_nets = 0;
    string line;

    while (getline(file, line)) {
        if (line.empty()) continue;

        if (line[0] == 'c') {
            continue;
        }

        if (line[0] == 'p') {
            istringstream iss(line);
            string p;
            string hypre;
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

// Parse IBM .netD format: header line, then pin records (a=module, p=pad, s=new net).
auto read_netD_format(const string& filename) -> SimpleNetlist {
    auto netD = ifstream{filename};
    if (netD.fail()) {
        cerr << "Error: Can't open file " << filename << ".\n";
        exit(1);
    }

    char t = 0;
    uint32_t numPins = 0;
    uint32_t numNets = 0;
    uint32_t numModules = 0;
    index_t padOffset = 0;

    netD >> t;
    netD >> numPins >> numNets >> numModules >> padOffset;

    const auto num_vertices = numModules + numNets;
    auto g = graph_t(num_vertices);

    constexpr index_t bufferSize = 100;
    char lineBuffer[bufferSize];
    netD.getline(lineBuffer, bufferSize);

    index_t w = 0;
    index_t e = numModules - 1;
    char c = 0;
    uint32_t i = 0;
    for (; i < numPins; ++i) {
        if (netD.eof()) {
            break;
        }
        do {
            netD.get(c);
        } while ((isspace(c) != 0));
        if (c == '\n') {
            continue;
        }
        if (c == 'a') {
            netD >> w;
        } else if (c == 'p') {
            netD >> w;
            w += padOffset;
        }
        do {
            netD.get(c);
        } while ((isspace(c) != 0));
        if (c == 's') {
            ++e;
        }

        g.add_edge(w, e);

        do {
            netD.get(c);
        } while ((isspace(c) != 0) && c != '\n');
        if (c != '\n') {
            netD.getline(lineBuffer, bufferSize);
        }
    }

    e -= numModules - 1;
    numNets = std::min(e, numNets);

    auto hyprgraph = SimpleNetlist{g, numModules, numNets};
    hyprgraph.num_pads = numModules - padOffset - 1;
    return hyprgraph;
}

// Dispatch to format-specific reader based on detected or explicit format.
auto read_hypergraph(const string& filename, InputFormat format) -> SimpleNetlist {
    auto actual_format = format;
    if (format == InputFormat::auto_detect) {
        actual_format = detect_input_format(filename);
    }

    switch (actual_format) {
        case InputFormat::hmetis:
            return read_hmetis_format(filename);
        case InputFormat::json:
            return read_json_format(filename);
        case InputFormat::dimacs:
            return read_dimacs_format(filename);
        case InputFormat::netD:
        case InputFormat::auto_detect:
            return read_netD_format(filename);
        default:
            cerr << "Error: Unknown input format.\n";
            exit(1);
    }
}

// Write partition in hMetis format: one integer per line.
void write_hmetis_partition(const vector<uint8_t>& part, ostream& os) {
    for (const auto p : part) {
        os << static_cast<int>(p) << "\n";
    }
}

// Write partition as a JSON array, e.g. [0, 1, 0, 1, ...].
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

// Dispatch partition output to the appropriate format writer.
void write_partition(const vector<uint8_t>& part, ostream& os, OutputFormat format) {
    if (format == OutputFormat::json) {
        write_json_partition(part, os);
    } else {
        write_hmetis_partition(part, os);
    }
}
