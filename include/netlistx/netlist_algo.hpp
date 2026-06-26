#pragma once

#include <algorithm>
#include <py2cpp/set.hpp>
#include <utility>

/**
 * @file netlist_algo.hpp
 * @brief Netlist-specific algorithms for hypergraph optimization
 *
 * This file contains algorithms specifically designed for netlist hypergraph optimization
 * problems, including minimum weighted vertex cover and maximum weighted matching.
 * These algorithms are commonly used in VLSI CAD tools for partitioning, placement,
 * and other optimization tasks.
 */

/**
 * @brief Solve minimum weighted vertex cover via primal-dual approximation
 *
 * Iterates over hypergraph nets, selecting the vertex with the minimum weight gap
 * to add to the cover set. Maintains dual variables for approximation guarantee.
 *
 * @dot
 *   digraph hypr_cover {
 *     rankdir=LR; bgcolor="transparent";
 *     node [shape=box, style=filled, fillcolor="#d4e6f1"];
 *     init [label="Init gaps\n= weights", fillcolor="#a9cce3"];
 *     nets [label="For each\nhyperedge"];
 *     min_v [label="Find min-gap\nvertex in net"];
 *     add [label="Add to\ncover set"];
 *     sub [label="Subtract gap\nfrom all vertices"];
 *     done [label="Cover!", fillcolor="#7fb3d8"];
 *     init -> nets -> min_v -> add -> sub -> nets;
 *     sub -> done [label="all nets\ncovered", color="#27ae60"];
 *   }
 * @enddot
 *
 * @tparam Gnl Hypergraph type
 * @tparam C1 Weight function type
 * @tparam C2 Cover set type
 * @param[in] hyprgraph The input hypergraph
 * @param[in] weight Vertex weight function
 * @param[in,out] coverset Pre-covered vertices; updated with the solution
 * @return Total primal cost of the cover
 */
template <typename Gnl, typename C1, typename C2>
auto min_vertex_cover(const Gnl& hyprgraph, const C1& weight, C2& coverset) ->
    typename C1::mapped_type {
    using T = typename C1::mapped_type;
    auto in_coverset = [&](const auto& v) { return coverset.contains(v); };
    [[maybe_unused]] auto total_dual_cost = T(0);
    auto total_primal_cost = T(0);
    auto gap = weight;
    for (const auto& net : hyprgraph.nets) {
        if (std::any_of(hyprgraph.gr[net].begin(), hyprgraph.gr[net].end(), in_coverset)) {
            continue;
        }

        auto min_vtx
            = *std::min_element(hyprgraph.gr[net].begin(), hyprgraph.gr[net].end(),
                                [&](const auto& v1, const auto& v2) { return gap[v1] < gap[v2]; });
        auto min_val = gap[min_vtx];
        coverset.insert(min_vtx);
        total_primal_cost += weight[min_vtx];
        total_dual_cost += min_val;
        for (const auto& u : hyprgraph.gr[net]) {
            gap[u] -= min_val;
        }
    }

    assert(total_dual_cost <= total_primal_cost);
    return total_primal_cost;
}

/**
 * @brief Solve minimum weighted maximal matching via primal-dual approximation
 *
 * Iterates through all nets, greedily selecting nets that do not share vertices
 * with already selected nets. For each candidate net, selects the one with minimum
 * gap value (modified weight).
 *
 * Time complexity: O(|V| * |E|^2)
 *
 * @tparam Gnl Hypergraph type
 * @tparam C1 Weight function type
 * @tparam C2 Match set / dependency set type
 * @param[in] hyprgraph Input hypergraph
 * @param[in] weight Weight function for nets
 * @param[in,out] matchset Output set of selected nets
 * @param[in,out] dep Output set of covered vertices
 * @return Total primal cost (sum of weights) of the matching
 */
template <typename Gnl, typename C1, typename C2>
auto min_maximal_matching(const Gnl& hyprgraph, const C1& weight, C2& matchset, C2& dep) ->
    typename C1::mapped_type;

/**
 * @brief Convenience overload that creates empty matchset/dependency sets
 *
 * @tparam Gnl Hypergraph type
 * @tparam C1 Weight function type
 * @param[in] hyprgraph Input hypergraph
 * @param[in] weight Weight function
 * @return Pair of (matching set, total cost)
 */
template <typename Gnl, typename C1>
auto min_maximal_matching(const Gnl& hyprgraph, const C1& weight)
    -> std::pair<py::set<typename Gnl::node_t>, typename C1::mapped_type>;
