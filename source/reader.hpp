#pragma once

/**
 * @file reader.hpp
 * @brief Internal (non-installed) strategy hierarchy for hypergraph file readers
 *
 * Implements the Strategy and Factory patterns for format-specific parsing.
 * The public API in <netlistx/readwrite.hpp> is intentionally unchanged: the
 * public free functions delegate to these internal classes. This header lives
 * in source/ so it is never installed or exported.
 */

#include <fstream>  // for ifstream
#include <memory>   // for unique_ptr
#include <netlistx/readwrite.hpp>
#include <string>       // for string
#include <string_view>  // for string_view

namespace netlistx::detail {

    /// Centralized fatal-error reporting. Prints @p msg to stderr and exits with @p code.
    [[noreturn]] void fail(const std::string& msg, int code = 1);

    /// Open an input file for reading; calls fail() if the file cannot be opened.
    auto open_input(std::string_view filename) -> std::ifstream;

    /**
     * @brief Strategy interface: parse one input format into a SimpleNetlist
     */
    class HypergraphReader {
      public:
        virtual ~HypergraphReader() = default;
        virtual auto read(std::string_view filename) const -> SimpleNetlist = 0;
    };

    /// hMetis format reader (.hgr, .graph)
    class HmetisReader final : public HypergraphReader {
      public:
        auto read(std::string_view filename) const -> SimpleNetlist override;
    };

    /// Generic JSON netlist reader (.json)
    class JsonReader final : public HypergraphReader {
      public:
        auto read(std::string_view filename) const -> SimpleNetlist override;
    };

    /// DIMACS hypergraph reader (.dimacs)
    class DimacsReader final : public HypergraphReader {
      public:
        auto read(std::string_view filename) const -> SimpleNetlist override;
    };

    /// IBM .netD/.net reader
    class NetDReader final : public HypergraphReader {
      public:
        auto read(std::string_view filename) const -> SimpleNetlist override;
    };

    /**
     * @brief Factory: create the reader matching @p format
     *
     * InputFormat::auto_detect falls back to the netD reader, matching the
     * historical dispatch in read_hypergraph() where an unrecognized extension
     * fell through to the netD parser.
     */
    auto make_reader(InputFormat format) -> std::unique_ptr<HypergraphReader>;

}  // namespace netlistx::detail
