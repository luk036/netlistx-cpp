/**
 * @file cover.hpp
 * @brief Primal-dual approximation algorithms for hypergraph covering
 */

#pragma once

#include <algorithm>
#include <cassert>
#include <deque>
#include <netlistx/gen.hpp>
#include <py2cpp/dict.hpp>
#include <py2cpp/set.hpp>
#include <queue>
#include <utility>
#include <vector>

/**
 * @brief Primal-dual approximation algorithm for covering problems
 *
 * Phase 1 selects vertices greedily using dual gap variables.
 * Phase 2 applies reverse-delete post-processing to remove redundant vertices.
 *
 * @dot
 *   digraph pd_cover_flow {
 *     bgcolor="transparent";
 *     rankdir=LR;
 *     node [shape=box, style=filled, fillcolor="#d4e6f1"];
 *     init [label="Init gap\n= weight", fillcolor="#a9cce3"];
 *     violate [label="Get\nviolate set"];
 *     min_v [label="Find min-gap\nvertex"];
 *     add [label="Add to\nsolution"];
 *     rd [label="Reverse-delete\npost-process", fillcolor="#f9e79f"];
 *     done [label="Cover!", fillcolor="#7fb3d8"];
 *     init -> violate -> min_v -> add -> violate;
 *     add -> rd [style=dashed, color="#888", constraint=false];
 *     rd -> done [color="#27ae60"];
 *   }
 * @enddot
 *
 * @tparam ViolateFunc Callable returning violate sets (edges/cycles not covered)
 * @tparam WeightMap Weight mapping type
 * @tparam SolutionSet Solution set type
 * @param[in] violate Callable returning sets of violating vertices
 * @param[in] weight Weight function for vertices
 * @param[in,out] soln Solution set, modified in place
 * @return Pair of (solution set, total primal cost)
 */
template <typename ViolateFunc, typename WeightMap, typename SolutionSet>
auto pd_cover(ViolateFunc violate, WeightMap& weight, SolutionSet& soln)
    -> std::pair<SolutionSet, typename WeightMap::mapped_type> {
    using CostType = typename WeightMap::mapped_type;
    using NodeType = typename SolutionSet::value_type;

    CostType total_dual_cost = 0;
    auto gap = weight;  // copy weights
    std::vector<NodeType> added_order;

    // Phase 1: Primal-Dual Selection
    for (auto&& violate_set : violate()) {
        if (violate_set.empty()) continue;

        auto min_vtx
            = *std::min_element(violate_set.begin(), violate_set.end(),
                                [&](const auto& v1, const auto& v2) { return gap[v1] < gap[v2]; });
        auto min_val = gap[min_vtx];

        if (!soln.contains(min_vtx)) {
            soln.insert(min_vtx);
            added_order.emplace_back(min_vtx);
        }

        total_dual_cost += min_val;

        for (const auto& vtx : violate_set) {
            gap[vtx] -= min_val;
        }
    }

    // Phase 2: Reverse-Delete Post-Processing
    for (auto it = added_order.rbegin(); it != added_order.rend(); ++it) {
        soln.erase(*it);
        bool is_redundant = true;
        for (auto&& check_set : violate()) {
            if (!check_set.empty()) {
                is_redundant = false;
                break;
            }
        }
        if (!is_redundant) {
            soln.insert(*it);
        }
    }

    CostType final_prml_cost = 0;
    for (const auto& vtx : soln) {
        final_prml_cost += weight[vtx];
    }

    assert(total_dual_cost <= final_prml_cost);
    return std::make_pair(soln, final_prml_cost);
}

/**
 * @brief Performs minimum weighted hypergraph vertex cover using primal-dual approximation.
 *
 * @dot
 *   digraph hyper_vc {
 *     bgcolor="transparent";
 *     rankdir=LR;
 *     node [shape=box, style=filled, fillcolor="#d4e6f1"];
 *     init [label="Init gaps\n= weights", fillcolor="#a9cce3"];
 *     nets [label="For each\nuncovered net"];
 *     min_v [label="Find min-gap\nvertex"];
 *     add [label="Add to\ncover set"];
 *     sub [label="Subtract gap\nfrom net vertices"];
 *     done [label="Cover!", fillcolor="#7fb3d8"];
 *     init -> nets -> min_v -> add -> sub -> nets;
 *     sub -> done [label="all covered", color="#27ae60"];
 *   }
 * @enddot
 *
 * @tparam Hypergraph Hypergraph type
 * @tparam WeightMap Weight map type
 * @tparam CoverSet Cover set type
 * @param hyprgraph Input hypergraph
 * @param weight Weight function
 * @param coverset Cover set (will be modified)
 * @return std::pair<CoverSet, typename WeightMap::mapped_type> Cover set and total weight
 */
template <typename Hypergraph, typename WeightMap, typename CoverSet>
auto min_hyper_vertex_cover(const Hypergraph& hyprgraph, WeightMap& weight, CoverSet& coverset)
    -> std::pair<CoverSet, typename WeightMap::mapped_type> {
    using node_t = typename Hypergraph::node_t;

    // Lambda function that generates violate nets (uncovered nets)
    auto violate_netlist = [&]() -> py::Generator<std::vector<node_t>> {
        for (const auto& net : hyprgraph.nets) {
            bool covered = false;
            for (const auto& vtx : hyprgraph.gr[net]) {
                if (coverset.contains(vtx)) {
                    covered = true;
                    break;
                }
            }

            if (!covered) {
                std::vector<node_t> net_vertices;
                for (const auto& vtx : hyprgraph.gr[net]) {
                    net_vertices.emplace_back(vtx);
                }
                co_yield std::move(net_vertices);
            }
        }
    };

    return pd_cover(violate_netlist, weight, coverset);
}

/**
 * @brief Convenience overload that creates an empty coverset internally.
 *
 * @tparam Hypergraph Hypergraph type
 * @tparam WeightMap Weight map type
 * @param hyprgraph Input hypergraph
 * @param weight Weight function
 * @return std::pair<py::set<typename Hypergraph::node_t>, typename WeightMap::mapped_type>
 *         Cover set and total weight
 */
template <typename Hypergraph, typename WeightMap>
auto min_hyper_vertex_cover(const Hypergraph& hyprgraph, WeightMap& weight)
    -> std::pair<py::set<typename Hypergraph::node_t>, typename WeightMap::mapped_type> {
    py::set<typename Hypergraph::node_t> coverset{};
    return min_hyper_vertex_cover(hyprgraph, weight, coverset);
}
