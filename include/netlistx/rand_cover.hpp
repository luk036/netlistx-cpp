#pragma once

/**
 * @file rand_cover.hpp
 * @brief Randomized Approximation Algorithm for Minimum Weighted Vertex Cover
 *
 * Implements Pitt's randomized algorithm (1985) for solving the minimum
 * weighted vertex cover problem. The algorithm achieves an expected
 * approximation ratio of 2.
 *
 * For each uncovered edge (u, v), selects one endpoint with probability
 * inversely proportional to its weight:
 *     P(pick u) = w(v) / (w(u) + w(v))
 *
 * Multi-threaded overloads run independent trials in parallel and return
 * the best (lowest-weight) cover, mirroring the GPU-accelerated Python
 * implementation in rand_cover_gpu.py.
 *
 * Reference:
 *     L. Pitt, "A Simple Probabilistic Approximation Algorithm for Vertex
 *     Cover," Technical Report, Yale University, 1985.
 */

#include <algorithm>
#include <cassert>
#include <future>
#include <optional>
#include <random>
#include <utility>
#include <vector>

#include <py2cpp/set.hpp>

#include <netlistx/thread_pool.hpp>

namespace detail {

/**
 * @brief Reverse-delete post-processing step.
 *
 * Iterates through vertices in reverse addition order. For each vertex,
 * temporarily removes it; if the cover remains valid, the removal is
 * kept (vertex was redundant). Otherwise the vertex is restored.
 *
 * @tparam Node Vertex type
 * @tparam Validator Callable that returns true if current cover is valid
 * @param soln Mutable cover set (modified in place)
 * @param added_order Vertices in order they were added
 * @param is_valid Validation callable
 */
template <typename Node, typename Validator>
void reverse_delete_cover(py::set<Node>& soln,
                           const std::vector<Node>& added_order,
                           Validator&& is_valid) {
    for (auto it = added_order.rbegin(); it != added_order.rend(); ++it) {
        soln.erase(*it);
        if (!std::forward<Validator>(is_valid)()) {
            soln.insert(*it);
        }
    }
}

} // namespace detail

/**
 * @brief Single trial of Pitt's randomized vertex cover algorithm.
 *
 * @tparam Graph Graph type (requires node_t, edges())
 * @tparam WeightMap Weight map type (requires mapped_type, operator[])
 * @tparam RNG Random number generator type
 * @param ugraph Input undirected graph
 * @param weight Vertex weight mapping
 * @param coverset Initial vertex cover (preserved in the result)
 * @param rng Random number generator (seeded externally)
 * @return std::pair<py::set<typename Graph::node_t>, typename WeightMap::mapped_type>
 */
template <typename Graph, typename WeightMap, typename RNG>
auto rand_vertex_cover_trial(const Graph& ugraph,
                              const WeightMap& weight,
                              const py::set<typename Graph::node_t>& coverset,
                              RNG& rng)
    -> std::pair<py::set<typename Graph::node_t>, typename WeightMap::mapped_type> {
    using node_t = typename Graph::node_t;
    using CostType = typename WeightMap::mapped_type;

    py::set<node_t> soln = coverset.copy();
    std::vector<node_t> added_order;
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    for (const auto& edge : ugraph.edges()) {
        auto u = edge.first;
        auto v = edge.second;

        if (soln.contains(u) || soln.contains(v)) {
            continue;
        }

        // Pitt's rule: pick u with prob w(v) / (w(u) + w(v))
        auto w_u = weight[u];
        auto w_v = weight[v];
        const double threshold = static_cast<double>(w_v) /
            (static_cast<double>(w_u) + static_cast<double>(w_v));

        if (dist(rng) < threshold) {
            soln.insert(u);
            added_order.push_back(u);
        } else {
            soln.insert(v);
            added_order.push_back(v);
        }
    }

    // Phase 2: Reverse-Delete Post-Processing
    auto is_covered = [&]() -> bool {
        for (const auto& e : ugraph.edges()) {
            if (!soln.contains(e.first) && !soln.contains(e.second)) {
                return false;
            }
        }
        return true;
    };
    detail::reverse_delete_cover(soln, added_order, is_covered);

    CostType total_cost{};
    for (const auto& v : soln) {
        total_cost += weight[v];
    }
    return {std::move(soln), total_cost};
}

// -----------------------------------------------------------------------
// Convenience overload — single trial with seed
// -----------------------------------------------------------------------

/**
 * @brief Single-trial randomized vertex cover (seeded convenience wrapper).
 *
 * @copydetails rand_vertex_cover_trial
 * @param ugraph Input graph
 * @param weight Vertex weights
 * @param seed Random seed (default: 0). Use std::nullopt for non-deterministic.
 * @param coverset Optional initial cover set
 */
template <typename Graph, typename WeightMap>
auto rand_vertex_cover(const Graph& ugraph,
                        const WeightMap& weight,
                        std::optional<unsigned int> seed = std::optional<unsigned int>{0},
                        const py::set<typename Graph::node_t>& coverset = {})
    -> std::pair<py::set<typename Graph::node_t>, typename WeightMap::mapped_type> {
    // using node_t = typename Graph::node_t;

    if (seed.has_value()) {
        std::mt19937 rng{seed.value()};
        return rand_vertex_cover_trial(ugraph, weight, coverset, rng);
    }
    std::random_device rd;
    std::mt19937 rng{rd()};
    return rand_vertex_cover_trial(ugraph, weight, coverset, rng);
}

// -----------------------------------------------------------------------
// Multi-threaded: run num_trials independent trials, return best cover
// -----------------------------------------------------------------------

/**
 * @brief Multi-threaded randomized vertex cover.
 *
 * Runs @p num_trials independent Pitt trials in parallel using
 * std::async, then returns the cover with the lowest total weight.
 * This is the C++20 equivalent of the GPU-accelerated Python version.
 *
 * @tparam Graph Graph type
 * @tparam WeightMap Weight map type
 * @param ugraph Input undirected graph
 * @param weight Vertex weight mapping (read-concurrently from threads)
 * @param num_trials Number of independent Monte Carlo trials (default: 64)
 * @param seed Master random seed (default: 0). Trials use seed + index.
 * @param coverset Optional initial cover set (shared by all trials)
 * @return std::pair<py::set<typename Graph::node_t>, typename WeightMap::mapped_type>
 */
template <typename Graph, typename WeightMap>
auto rand_vertex_cover_mt(const Graph& ugraph,
                           const WeightMap& weight,
                           unsigned int num_trials = 64,
                           unsigned int seed = 0,
                           const py::set<typename Graph::node_t>& coverset = {})
    -> std::pair<py::set<typename Graph::node_t>, typename WeightMap::mapped_type> {
    using node_t = typename Graph::node_t;
    using CostType = typename WeightMap::mapped_type;
    using Result = std::pair<py::set<node_t>, CostType>;

    netlistx::thread_pool pool;
    std::vector<std::future<Result>> futures;
    futures.reserve(num_trials);

    for (unsigned int t = 0; t < num_trials; ++t) {
        futures.push_back(pool.enqueue([&, t]() -> Result {
            std::mt19937 rng{seed + t};
            return rand_vertex_cover_trial(ugraph, weight, coverset, rng);
        }));
    }

    // Collect results and keep the best (lowest cost)
    auto best = futures[0].get();
    for (unsigned int t = 1; t < num_trials; ++t) {
        auto result = futures[t].get();
        if (result.second < best.second) {
            best = std::move(result);
        }
    }

    return best;
}

// -----------------------------------------------------------------------
// Hypergraph versions
// -----------------------------------------------------------------------

/**
 * @brief Single trial of Pitt's randomized vertex cover for hypergraphs.
 *
 * A hyperedge (net) can connect multiple vertices. For each uncovered net
 * {v1, ..., vk}, the algorithm picks vertex vi with probability inversely
 * proportional to its weight:
 *     P(pick vi) = (1/w(vi)) / sum(1/w(vj) for vj in net)
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
auto rand_hyper_vertex_cover_trial(const Hypergraph& hyprgraph,
                                    const WeightMap& weight,
                                    const py::set<typename Hypergraph::node_t>& coverset,
                                    RNG& rng)
    -> std::pair<py::set<typename Hypergraph::node_t>, typename WeightMap::mapped_type> {
    using node_t = typename Hypergraph::node_t;
    using CostType = typename WeightMap::mapped_type;

    py::set<node_t> soln = coverset.copy();
    std::vector<node_t> added_order;
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    for (const auto& net : hyprgraph.nets) {
        const auto& vertices = hyprgraph.gr[net];

        // Skip nets already covered
        bool covered = false;
        for (const auto& v : vertices) {
            if (soln.contains(v)) {
                covered = true;
                break;
            }
        }
        if (covered || vertices.empty()) {
            continue;
        }

        // Generalized Pitt rule: P(pick vi) ∝ 1/w(vi)
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
    auto is_covered = [&]() -> bool {
        for (const auto& net : hyprgraph.nets) {
            bool net_covered = false;
            for (const auto& v : hyprgraph.gr[net]) {
                if (soln.contains(v)) {
                    net_covered = true;
                    break;
                }
            }
            if (!net_covered && !hyprgraph.gr[net].empty()) {
                return false;
            }
        }
        return true;
    };
    detail::reverse_delete_cover(soln, added_order, is_covered);

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
auto rand_hyper_vertex_cover(
    const Hypergraph& hyprgraph,
    const WeightMap& weight,
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
auto rand_hyper_vertex_cover_mt(
    const Hypergraph& hyprgraph,
    const WeightMap& weight,
    unsigned int num_trials = 64,
    unsigned int seed = 0,
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
