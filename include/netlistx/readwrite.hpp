#pragma once

#include <cstdint>
#include <iosfwd>
#include <netlistx/netlist.hpp>
#include <string>
#include <string_view>
#include <vector>

enum class InputFormat { hmetis, json, dimacs, netD, auto_detect };

enum class OutputFormat { hmetis, json };

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

void writeJSON(std::string_view jsonFileName, const SimpleNetlist& hyprgraph);

auto readNetD(std::string_view netDFileName) -> SimpleNetlist;

void readAre(SimpleNetlist& hyprgraph, std::string_view areFileName);

auto detect_input_format(const std::string& filename) -> InputFormat;

auto read_hmetis_format(const std::string& filename) -> SimpleNetlist;

auto read_json_format(const std::string& filename) -> SimpleNetlist;

auto read_dimacs_format(const std::string& filename) -> SimpleNetlist;

auto read_netD_format(const std::string& filename) -> SimpleNetlist;

auto read_hypergraph(const std::string& filename, InputFormat format = InputFormat::auto_detect)
    -> SimpleNetlist;

void write_hmetis_partition(const std::vector<std::uint8_t>& part, std::ostream& os);

void write_json_partition(const std::vector<std::uint8_t>& part, std::ostream& os);

void write_partition(const std::vector<std::uint8_t>& part, std::ostream& os,
                     OutputFormat format = OutputFormat::hmetis);
