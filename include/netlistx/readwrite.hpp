#pragma once

#include <cstdint>
#include <iosfwd>
#include <netlistx/netlist.hpp>
#include <string>
#include <string_view>
#include <vector>

/**
 * @brief Supported input file formats for reading hypergraphs.
 *
 * Used by read_hypergraph() to determine the parser to use.
 * auto_detect attempts to infer the format from the file extension.
 */
enum class InputFormat {
    hmetis,      ///< hMetis format (.hgr, .graph)
    json,        ///< JSON format (.json)
    dimacs,      ///< DIMACS format (.dimacs)
    netD,        ///< IBM .netD format (.net)
    auto_detect  ///< Infer format from file extension
};

/**
 * @brief Supported output file formats for writing partitions.
 */
enum class OutputFormat {
    hmetis,  ///< hMetis-style (one value per line)
    json     ///< JSON array format
};

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

/**
 * @brief Write a SimpleNetlist to a JSON file.
 *
 * Serializes the netlist structure (modules, nets, links) into a JSON format
 * compatible with common graph visualization tools.
 *
 * @param jsonFileName Path to the output JSON file
 * @param hyprgraph The SimpleNetlist to serialize
 */
void writeJSON(std::string_view jsonFileName, const SimpleNetlist& hyprgraph);

/**
 * @brief Read an IBM .netD/.net format file into a SimpleNetlist.
 *
 * Parses the standard IBM .netD netlist format used in VLSI CAD benchmarks.
 * Constructs a bipartite graph with module and net nodes from the pin
 * connectivity data.
 *
 * @param netDFileName Path to the input .netD/.net file
 * @return SimpleNetlist constructed from the file data
 */
auto readNetD(std::string_view netDFileName) -> SimpleNetlist;

/**
 * @brief Read an IBM .are format file and populate module weights.
 *
 * Parses the IBM .are (area/weight) file format and assigns weights
 * to the corresponding modules in the netlist.
 *
 * @param hyprgraph The SimpleNetlist to populate with weights (modified in-place)
 * @param areFileName Path to the input .are file
 */
void readAre(SimpleNetlist& hyprgraph, std::string_view areFileName);

/**
 * @brief Detect the input file format from its extension.
 *
 * Maps file extensions to InputFormat values:
 *   .net  -> netD, .hgr/.graph -> hmetis, .json -> json, .dimacs -> dimacs
 *
 * @param filename The input file path
 * @return InputFormat enum value, or auto_detect if unrecognized
 */
auto detect_input_format(const std::string& filename) -> InputFormat;

/**
 * @brief Read a hypergraph from an hMetis format file.
 *
 * Parses the hMetis format where the first line specifies
 * the number of nets and vertices, followed by one line per net
 * listing its vertex connections.
 *
 * @param filename Path to the input .hgr or .graph file
 * @return SimpleNetlist constructed from the file data
 */
auto read_hmetis_format(const std::string& filename) -> SimpleNetlist;

/**
 * @brief Read a hypergraph from a JSON format file.
 *
 * Parses the JSON netlist format with "graph" metadata
 * (num_modules, num_nets, num_pads) and "links" array.
 *
 * @param filename Path to the input .json file
 * @return SimpleNetlist constructed from the file data
 */
auto read_json_format(const std::string& filename) -> SimpleNetlist;

/**
 * @brief Read a hypergraph from a DIMACS format file.
 *
 * Parses the DIMACS hypergraph format with problem line
 * "p hypre V E" defining vertex and hyperedge counts.
 *
 * @param filename Path to the input .dimacs file
 * @return SimpleNetlist constructed from the file data
 */
auto read_dimacs_format(const std::string& filename) -> SimpleNetlist;

/**
 * @brief Read a hypergraph from an IBM .netD format file.
 *
 * Alternative entry point that opens the file by path string
 * rather than an already-open stream.
 *
 * @param filename Path to the input .net file
 * @return SimpleNetlist constructed from the file data
 */
auto read_netD_format(const std::string& filename) -> SimpleNetlist;

/**
 * @brief Read a hypergraph from a file, auto-detecting the format.
 *
 * Convenience function that detects the input format (or uses an explicit
 * format override) and dispatches to the appropriate reader.
 *
 * @param filename Path to the input file
 * @param format Input format override (default: auto_detect)
 * @return SimpleNetlist constructed from the file data
 */
auto read_hypergraph(const std::string& filename, InputFormat format = InputFormat::auto_detect)
    -> SimpleNetlist;

/**
 * @brief Write a partition vector in hMetis format.
 *
 * Outputs one integer per line, where each line represents the
 * partition assignment of the corresponding module.
 *
 * @param part Partition assignment vector (one entry per module)
 * @param os Output stream to write to
 */
void write_hmetis_partition(const std::vector<std::uint8_t>& part, std::ostream& os);

/**
 * @brief Write a partition vector in JSON array format.
 *
 * Outputs the partition as a JSON array, e.g. [0, 1, 0, 1, ...].
 *
 * @param part Partition assignment vector (one entry per module)
 * @param os Output stream to write to
 */
void write_json_partition(const std::vector<std::uint8_t>& part, std::ostream& os);

/**
 * @brief Write a partition vector to a stream in the specified format.
 *
 * Dispatches to write_hmetis_partition() or write_json_partition()
 * based on the output format.
 *
 * @param part Partition assignment vector (one entry per module)
 * @param os Output stream to write to
 * @param format Output format (default: hmetis)
 */
void write_partition(const std::vector<std::uint8_t>& part, std::ostream& os,
                     OutputFormat format = OutputFormat::hmetis);
