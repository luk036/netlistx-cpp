#pragma once

#include <netlistx/netlist.hpp>
#include <string_view>

/**
 * @brief Read a Yosys JSON file and convert it to a SimpleNetlist object.
 *
 * Parses Yosys synthesis output JSON and constructs a bipartite graph
 * where cells and I/O ports are module nodes, and wires are net nodes.
 *
 * String-valued net IDs (constants like "0", "1") are skipped.
 * I/O port nodes are assigned weight 0 and marked as fixed.
 *
 * @param filename Path to Yosys JSON file
 * @return SimpleNetlist object representing the circuit
 */
auto read_yosys_json(std::string_view filename) -> SimpleNetlist;
