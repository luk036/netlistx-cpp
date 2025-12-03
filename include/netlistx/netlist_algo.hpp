#pragma once

#include <algorithm>

/**
 * @file netlist_algo.hpp
 * @brief Netlist-specific algorithms for hypergraph optimization
 * 
 * This file contains algorithms specifically designed for netlist hypergraph optimization
 * problems, including minimum weighted vertex cover and maximum weighted matching.
 * These algorithms are commonly used in VLSI CAD tools for partitioning, placement,
 * and other optimization tasks.
 */

// #include <range/v3/algorithm/any_of.hpp>
// #include <range/v3/algorithm/min_element.hpp>

/**
 * @brief Solves the minimum weighted vertex cover problem using the primal-dual paradigm.
 *
 * This function takes a hypergraph, a weight function, and a set of pre-covered vertices. It
 * computes the minimum weighted vertex cover by iterating over the hypergraph's nets and
 * selecting the vertex with the minimum weight gap to add to the cover set. The total primal
 * and dual costs are computed and returned.
 *
 * @tparam Gnl The type of the hypergraph.
 * @tparam C1 The type of the weight function.
 * @tparam C2 The type of the cover set.
 * @param hyprgraph The input hypergraph.
 * @param weight The weight function.
 * @param[in,out] coverset The set of pre-covered vertices, which will be updated with the
 *                         solution set.
 * @return The total primal cost of the minimum weighted vertex cover.
 */
template <typename Gnl, typename C1, typename C2>
auto min_vertex_cover(const Gnl &hyprgraph, const C1 &weight, C2 &coverset) ->
    typename C1::mapped_type {
    using T = typename C1::mapped_type;
    auto in_coverset = [&](const auto &v) { return coverset.contains(v); };
    [[maybe_unused]] auto total_dual_cost = T(0);
    auto total_primal_cost = T(0);
    auto gap = weight;
    for (const auto &net : hyprgraph.nets) {
        if (std::any_of(hyprgraph.gr[net].begin(), hyprgraph.gr[net].end(), in_coverset)) {
            continue;
        }

        auto min_vtx
            = *std::min_element(hyprgraph.gr[net].begin(), hyprgraph.gr[net].end(),
                                [&](const auto &v1, const auto &v2) { return gap[v1] < gap[v2]; });
        auto min_val = gap[min_vtx];
        coverset.insert(min_vtx);
        total_primal_cost += weight[min_vtx];
        total_dual_cost += min_val;
        for (const auto &u : hyprgraph.gr[net]) {
            gap[u] -= min_val;
        }
    }

    assert(total_dual_cost <= total_primal_cost);
    return total_primal_cost;
}

/**
 * @brief Solves the minimum weighted maximal matching problem using the primal-dual paradigm.
 *
 * This function implements a primal-dual approximation algorithm for finding a minimum weighted
 * maximal matching in a hypergraph. The algorithm iterates through all nets in the hypergraph
 * and greedily selects nets that do not share vertices with already selected nets (maintained
 * in the dependency set). For each candidate net, it considers alternative nets that share
 * vertices and selects the one with minimum gap value (modified weight).
 * 
 * The algorithm maintains:
 * - A gap function representing the dual variables
 * - A matchset containing the selected nets
 * - A dependency set containing vertices covered by selected nets
 * 
 * Time complexity: O(|V| * |E|^2) where V is the set of vertices and E is the set of nets
 * 
 * @tparam Gnl The type of the hypergraph.
 * @tparam C1 The type of the weight function (must support [] operator and mapped_type).
 * @tparam C2 The type of the matching set and dependency set.
 * @param hyprgraph The input hypergraph containing nets and their vertex connections.
 * @param weight The weight function assigning weights to nets.
 * @param[in,out] matchset The output set containing the selected nets in the matching.
 * @param[in,out] dep The output set containing vertices covered by the matching.
 * @return typename C1::mapped_type The total primal cost (sum of weights) of the matching.
 */
template <typename Gnl, typename C1, typename C2>
auto min_maximal_matching(const Gnl &hyprgraph, const C1 &weight, C2 &matchset, C2 &dep) ->
    typename C1::mapped_type {
    /// Lambda function to mark all vertices of a net as dependent
    auto cover = [&](const auto &net) {
        for (const auto &v : hyprgraph.gr[net]) {
            dep.insert(v);
        }
    };

    /// Lambda function to check if a vertex is in the dependency set
    auto in_dep = [&](const auto &v) { return dep.contains(v); };

    // auto any_of_dep = [&](const auto& net) {
    //     return ranges::any_of(
    //         hyprgraph.gr[net], [&](const auto& v) { return dep.contains(v); });
    // };

    using T = typename C1::mapped_type;

    auto gap = weight;
    [[maybe_unused]] auto total_dual_cost = T(0);
    auto total_primal_cost = T(0);
    for (const auto &net : hyprgraph.nets) {
        if (std::any_of(hyprgraph.gr[net].begin(), hyprgraph.gr[net].end(), in_dep)) {
            continue;
        }
        if (matchset.contains(net)) {  // pre-define independant
            cover(net);
            continue;
        }
        auto min_val = gap[net];
        auto min_net = net;
        for (const auto &v : hyprgraph.gr[net]) {
            for (const auto &net2 : hyprgraph.gr[v]) {
                if (std::any_of(hyprgraph.gr[net2].begin(), hyprgraph.gr[net2].end(), in_dep)) {
                    continue;
                }
                if (min_val > gap[net2]) {
                    min_val = gap[net2];
                    min_net = net2;
                }
            }
        }
        cover(min_net);
        matchset.insert(min_net);
        total_primal_cost += weight[min_net];
        total_dual_cost += min_val;
        if (min_net != net) {
            gap[net] -= min_val;
            for (const auto &v : hyprgraph.gr[net]) {
                for (const auto &net2 : hyprgraph.gr[v]) {
                    gap[net2] -= min_val;
                }
            }
        }
    }
    // assert(total_dual_cost <= total_primal_cost);
    return total_primal_cost;
}

/**
 * @brief Convenience overload for min_maximal_matching that creates empty sets.
 *
 * This overload creates empty matchset and dependency sets and calls the main
 * min_maximal_matching function.
 *
 * @tparam Gnl The type of the hypergraph.
 * @tparam C1 The type of the weight function.
 * @param hyprgraph The input hypergraph.
 * @param weight The weight function.
 * @return std::pair<C2, typename C1::mapped_type> Pair containing the matching set and total cost.
 */
template <typename Gnl, typename C1>
auto min_maximal_matching(const Gnl &hyprgraph, const C1 &weight) ->
    std::pair<py::set<typename Gnl::node_t>, typename C1::mapped_type> {
    py::set<typename Gnl::node_t> matchset{};
    py::set<typename Gnl::node_t> dep{};
    auto cost = min_maximal_matching(hyprgraph, weight, matchset, dep);
    return std::make_pair(matchset, cost);
}
