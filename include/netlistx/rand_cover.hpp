#pragma once

/**
 * @file rand_cover.hpp
 * @brief Randomized Approximation Algorithm for Minimum Weighted Vertex Cover
 *
 * Implements Pitt's randomized algorithm (1985) for solving the minimum
 * weighted vertex cover problem on hypergraphs. The algorithm achieves an
 * expected approximation ratio of 2.
 *
 * For each uncovered hyperedge (net) {v1, ..., vk}, selects one endpoint
 * with probability inversely proportional to its weight:
 *     P(pick vi) = (1/w(vi)) / sum(1/w(vj) for vj in net)
 *
 * Multi-threaded overloads run independent trials in parallel and return
 * the best (lowest-weight) cover.
 *
 * Reference:
 *     L. Pitt, "A Simple Probabilistic Approximation Algorithm for Vertex
 *     Cover," Technical Report, Yale University, 1985.
 */

#include <algorithm>
#include <cassert>
#include <future>
#include <netlistx/detail/cover_util.hpp>
#include <netlistx/thread_pool.hpp>
#include <optional>
#include <py2cpp/set.hpp>
#include <random>
#include <utility>
#include <vector>

/**
 * @brief Single trial of Pitt's randomized hypergraph vertex cover.
 *
 * A hyperedge (net) can connect multiple vertices. For each uncovered net
 * \f$\{v_1, \dots, v_k\}\f$, the algorithm picks vertex \f$v_i\f$ with probability
 * inversely proportional to its weight:
 * @f[
 *     P(\text{pick } v_i) = \frac{1/w(v_i)}{\sum_{j=1}^{k} 1/w(v_j)}
 * @f]
 *
 * @dot
 *   digraph rand_trial {
 *     bgcolor="transparent";
 *     rankdir=LR;
 *     node [shape=box, style=filled, fillcolor="#d4e6f1"];
 *     init [label="Init soln\n= coverset", fillcolor="#a9cce3"];
 *     net_loop [label="For each\nuncovered net"];
 *     pick [label="Pick vertex vi\nP ~ 1/w(vi)", fillcolor="#f9e79f"];
 *     add [label="Add to\nsolution"];
 *     rd [label="Reverse-delete\npost-process"];
 *     done [label="Best cover\nfound!", fillcolor="#7fb3d8"];
 *     init -> net_loop -> pick -> add -> net_loop;
 *     add -> rd -> done;
 *   }
 * @enddot
 *
 * @tparam Hypergraph Hypergraph type (requires: nets, gr[net] -> vertices)
 * @tparam WeightMap Weight map type
 * @tparam RNG Random number generator type
 * @param hyprgraph Input hypergraph
 * @param weight Vertex weight mapping
 * @param coverset Initial cover set
 * @param rng Random number generator
 * @return std::pair<py::set<typename Hypergraph::node_t>, typename WeightMap::mapped_type>
 */
template <typename Hypergraph, typename WeightMap, typename RNG>
auto rand_hyper_vertex_cover_trial(const Hypergraph& hyprgraph, const WeightMap& weight,
                                   const py::set<typename Hypergraph::node_t>& coverset, RNG& rng)
    -> std::pair<py::set<typename Hypergraph::node_t>, typename WeightMap::mapped_type> {
    using node_t = typename Hypergraph::node_t;
    using CostType = typename WeightMap::mapped_type;

    py::set<node_t> soln = coverset.copy();
    std::vector<node_t> added_order;
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    for (const auto& net : hyprgraph.nets) {
        // Skip nets that are empty or already covered.
        if (hyprgraph.gr[net].empty() || netlistx::detail::net_is_covered(hyprgraph, net, soln)) {
            continue;
        }

        // Copy vertices to a vector for random access
        std::vector<node_t> vertices(hyprgraph.gr[net].begin(), hyprgraph.gr[net].end());

        // Generalized Pitt rule: P(pick vi) ~ 1/w(vi)
        double total_inv{};
        std::vector<double> inv_weights;
        inv_weights.reserve(vertices.size());
        for (const auto& v : vertices) {
            const double inv = 1.0 / static_cast<double>(weight[v]);
            inv_weights.push_back(inv);
            total_inv += inv;
        }

        const double r = dist(rng);
        double cumulative{};
        node_t chosen{};
        for (std::size_t i = 0; i < vertices.size(); ++i) {
            cumulative += inv_weights[i] / total_inv;
            if (r < cumulative) {
                chosen = vertices[i];
                break;
            }
        }

        soln.insert(chosen);
        added_order.push_back(chosen);
    }

    // Phase 2: Reverse-Delete Post-Processing
    netlistx::detail::reverse_delete(
        soln, added_order, [&]() { return netlistx::detail::all_nets_covered(hyprgraph, soln); });

    CostType total_cost{};
    for (const auto& v : soln) {
        total_cost += weight[v];
    }
    return {std::move(soln), total_cost};
}

/**
 * @brief Single-trial hypergraph vertex cover (seeded convenience wrapper).
 */
template <typename Hypergraph, typename WeightMap>
auto rand_hyper_vertex_cover(const Hypergraph& hyprgraph, const WeightMap& weight,
                             std::optional<unsigned int> seed = std::optional<unsigned int>{0},
                             const py::set<typename Hypergraph::node_t>& coverset = {})
    -> std::pair<py::set<typename Hypergraph::node_t>, typename WeightMap::mapped_type> {
    // using node_t = typename Hypergraph::node_t;

    if (seed.has_value()) {
        std::mt19937 rng{seed.value()};
        return rand_hyper_vertex_cover_trial(hyprgraph, weight, coverset, rng);
    }
    std::random_device rd;
    std::mt19937 rng{rd()};
    return rand_hyper_vertex_cover_trial(hyprgraph, weight, coverset, rng);
}

/**
 * @brief Multi-threaded hypergraph vertex cover.
 *
 * Runs @p num_trials independent trials and returns the best cover.
 */
template <typename Hypergraph, typename WeightMap>
auto rand_hyper_vertex_cover_mt(const Hypergraph& hyprgraph, const WeightMap& weight,
                                unsigned int num_trials = 64, unsigned int seed = 0,
                                const py::set<typename Hypergraph::node_t>& coverset = {})
    -> std::pair<py::set<typename Hypergraph::node_t>, typename WeightMap::mapped_type> {
    using node_t = typename Hypergraph::node_t;
    using CostType = typename WeightMap::mapped_type;
    using Result = std::pair<py::set<node_t>, CostType>;

    netlistx::thread_pool pool;
    std::vector<std::future<Result>> futures;
    futures.reserve(num_trials);

    for (unsigned int t = 0; t < num_trials; ++t) {
        futures.push_back(pool.enqueue([&, t]() -> Result {
            std::mt19937 rng{seed + t};
            return rand_hyper_vertex_cover_trial(hyprgraph, weight, coverset, rng);
        }));
    }

    auto best = futures[0].get();
    for (unsigned int t = 1; t < num_trials; ++t) {
        auto result = futures[t].get();
        if (result.second < best.second) {
            best = std::move(result);
        }
    }

    return best;
}
