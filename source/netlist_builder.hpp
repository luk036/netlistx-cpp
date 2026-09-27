#pragma once

/**
 * @file netlist_builder.hpp
 * @brief Internal (non-installed) factory that assembles a fully-consistent Netlist
 *
 * Readers parse different file formats into a raw graph plus per-format metadata
 * (num_pads, module weights, fixed modules). Instead of constructing a Netlist and
 * then mutating its public members afterwards at every call site, they hand all
 * pieces to make_netlist(), which produces the object in one place and derives the
 * dependent flag has_fixed_modules exactly once.
 *
 * This header lives in source/ and is never installed: it is only used by this
 * library's own readers, and the public API in <netlistx/netlist.hpp> is unchanged.
 */

#include <cassert>               // for assert
#include <cstdint>               // for uint32_t
#include <netlistx/netlist.hpp>  // for Netlist
#include <py2cpp/set.hpp>        // for set
#include <utility>               // for move
#include <vector>                // for vector

namespace netlistx::detail {

    /**
     * @brief Assemble a Netlist from a parsed graph plus optional netlist metadata.
     *
     * Constructs the Netlist through the standard count constructor (which computes
     * max_degree / max_net_degree) and then applies the remaining metadata in one
     * place: num_pads, the per-module weight vector, the fixed-module set, and the
     * derived has_fixed_modules flag (has_fixed_modules == !module_fixed.empty()).
     *
     * An empty module_weight vector keeps the Netlist default semantics
     * (get_module_weight() returns 1 for every module).
     *
     * @tparam Graph The underlying graph type (e.g. xnetwork::SimpleGraph)
     * @param[in] gr The graph to move into the Netlist
     * @param[in] num_modules Number of module nodes in @p gr
     * @param[in] num_nets Number of net nodes in @p gr
     * @param[in] num_pads Number of pad (I/O) modules; default 0
     * @param[in] module_weight Per-module weights; empty means uniform weight 1
     * @param[in] module_fixed Modules that are fixed in place; empty means none fixed
     * @return A fully-assembled Netlist with consistent derived state
     */
    template <typename Graph>
    auto make_netlist(Graph gr, uint32_t num_modules, uint32_t num_nets, size_t num_pads = 0U,
                      std::vector<unsigned int> module_weight = {},
                      py::set<typename Netlist<Graph>::node_t> module_fixed = {})
        -> Netlist<Graph> {
        assert(module_weight.empty() || module_weight.size() == num_modules);

        auto result = Netlist<Graph>{std::move(gr), num_modules, num_nets};
        result.num_pads = num_pads;
        result.module_weight = std::move(module_weight);
        result.module_fixed = std::move(module_fixed);
        result.has_fixed_modules = !result.module_fixed.empty();
        return result;
    }

}  // namespace netlistx::detail
