#pragma once

/**
 * @file rand_cover_gpu.hpp
 * @brief GPU-Accelerated Randomized Vertex Cover using Pitt's Algorithm
 *
 * Ported from Python `rand_cover_gpu.py` and Rust `rand_cover_gpu.rs`.
 *
 * Runs multiple independent Pitt trials in parallel on the GPU via CUDA.
 * Each CUDA thread executes one complete Pitt trial (edge iteration + random
 * vertex selection). Falls back to CPU multi-threaded execution when CUDA
 * is unavailable.
 *
 * @note Compile with `-DHAS_CUDA` to enable GPU acceleration.
 *       Requires CUDA toolkit and nvcc.
 *
 * Reference:
 *     L. Pitt, "A Simple Probabilistic Approximation Algorithm for Vertex
 *     Cover," Technical Report, Yale University, 1985.
 */

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <random>
#include <utility>
#include <vector>

namespace netlistx {

    namespace detail {

        /// Convert a graph's adjacency to a flat edge array for GPU processing.
        template <typename Graph> auto make_flat_edges(const Graph& grph, int num_edges)
            -> std::vector<int> {
            std::vector<int> edges;
            edges.reserve(num_edges * 2);
            for (const auto& edge : grph.edges()) {
                edges.push_back(static_cast<int>(edge.first));
                edges.push_back(static_cast<int>(edge.second));
            }
            return edges;
        }

        /// Extract weights as a flat float array, mapping node names to indices.
        template <typename Graph, typename WeightMap>
        auto make_flat_weights(const Graph& /*grph*/, int num_vertices, const WeightMap& weight)
            -> std::vector<float> {
            std::vector<float> wts(num_vertices);
            for (int i = 0; i < num_vertices; ++i) {
                auto it = weight.find(i);
                wts[i] = (it != weight.end()) ? static_cast<float>(it->second) : 1.0f;
            }
            return wts;
        }

#ifndef HAS_CUDA

        /** @brief CPU fallback: run a single Pitt trial. */
        inline void pitt_trial_cpu(const std::vector<int>& edges, int num_edges,
                                   const std::vector<float>& weights, int /*num_vertices*/,
                                   std::vector<unsigned int>& cover, int /*num_words*/,
                                   unsigned long long seed) {
            for (int i = 0; i < num_edges; ++i) {
                const int u = edges[i * 2];
                const int v = edges[i * 2 + 1];

                const int u_word = u >> 5;
                const int v_word = v >> 5;
                const unsigned int u_bit = 1u << (u & 31);
                const unsigned int v_bit = 1u << (v & 31);

                if ((cover[u_word] & u_bit) == 0 && (cover[v_word] & v_bit) == 0) {
                    seed = (seed * 1103515245ULL + 12345ULL) & 0x7FFFFFFFULL;
                    const double rand_val = static_cast<double>(seed) / 2147483648.0;

                    const float w_u = weights[u];
                    const float w_v = weights[v];
                    const float threshold = w_v / (w_u + w_v);

                    if (rand_val < threshold) {
                        cover[u_word] |= u_bit;
                    } else {
                        cover[v_word] |= v_bit;
                    }
                }
            }
        }

        /** @brief Compute cost from bitmask cover. */
        inline float compute_cost(const std::vector<unsigned int>& cover,
                                  const std::vector<float>& weights, int /*num_words*/) {
            float cost = 0.0f;
            for (int v = 0; v < static_cast<int>(weights.size()); ++v) {
                if (cover[v >> 5] & (1u << (v & 31))) {
                    cost += weights[v];
                }
            }
            return cost;
        }

        /** @brief Extract vertex set from bitmask. */
        template <typename Node> auto extract_cover_set(const std::vector<unsigned int>& cover,
                                                        const std::vector<Node>& vertices,
                                                        int /*num_words*/) -> std::vector<Node> {
            std::vector<Node> soln;
            for (int vi = 0; vi < static_cast<int>(vertices.size()); ++vi) {
                if (cover[vi >> 5] & (1u << (vi & 31))) {
                    soln.push_back(vertices[vi]);
                }
            }
            return soln;
        }

#endif  // !HAS_CUDA

    }  // namespace detail

#ifdef HAS_CUDA

    // Forward declaration: host-side CUDA launch (defined in rand_cover_gpu.cu).
    // Uses extern "C" with flat pointers for MSVC/nvcc cross-compiler linking.
    extern "C" {
    void run_gpu_trials_raw(const int* edges_flat, int num_edges, const float* weights_flat,
                            int num_vertices, const unsigned long long* seeds, int num_trials,
                            int num_words, unsigned int* out_covers, float* out_costs);
    }

    template <typename Graph, typename WeightMap>
    auto rand_vertex_cover_gpu(const Graph& grph, const WeightMap& weight, int num_vertices,
                               int num_edges, unsigned int seed = 42u, int num_trials = 1024)
        -> std::pair<std::vector<int>, float> {
        const auto edges_flat = detail::make_flat_edges(grph, num_edges);
        const auto weights_flat = detail::make_flat_weights(grph, num_vertices, weight);
        const int num_words = (num_vertices + 31) / 32;

        std::mt19937 rng{seed};
        std::vector<unsigned long long> seeds(static_cast<std::size_t>(num_trials));
        for (auto& s : seeds) {
            s = static_cast<unsigned long long>(rng())
                | (static_cast<unsigned long long>(rng()) << 32);
        }

        const int cover_sz = num_words * num_trials;
        std::vector<unsigned int> covers_host(cover_sz, 0u);
        std::vector<float> costs_host(num_trials);

        run_gpu_trials_raw(edges_flat.data(), num_edges, weights_flat.data(), num_vertices,
                           seeds.data(), num_trials, num_words, covers_host.data(),
                           costs_host.data());

        const auto best_idx = static_cast<int>(std::distance(
            costs_host.begin(), std::min_element(costs_host.begin(), costs_host.end())));

        std::vector<int> soln;
        const unsigned int* best_mask = &covers_host[best_idx * num_words];
        for (int vi = 0; vi < num_vertices; ++vi) {
            if (best_mask[vi >> 5] & (1u << (vi & 31))) soln.push_back(vi);
        }
        return {soln, costs_host[best_idx]};
    }

#else  // !HAS_CUDA

    template <typename Graph, typename WeightMap>
    auto rand_vertex_cover_gpu(const Graph& grph, const WeightMap& weight, int num_vertices,
                               int num_edges, unsigned int seed = 42u, int num_trials = 1024)
        -> std::pair<std::vector<int>, float> {
        const auto edges_flat = detail::make_flat_edges(grph, num_edges);
        const auto weights_flat = detail::make_flat_weights(grph, num_vertices, weight);
        const int num_words = (num_vertices + 31) / 32;

        std::mt19937 rng{seed};
        std::vector<unsigned long long> seeds(static_cast<std::size_t>(num_trials));
        for (auto& s : seeds) {
            s = static_cast<unsigned long long>(rng())
                | (static_cast<unsigned long long>(rng()) << 32);
        }

        std::vector<unsigned int> initial_cover(num_words, 0u);
        float best_cost = 1e30f;
        std::vector<int> best_soln;

        for (int t = 0; t < num_trials; ++t) {
            auto cover = initial_cover;
            detail::pitt_trial_cpu(edges_flat, num_edges, weights_flat, num_vertices, cover,
                                   num_words, seeds[t]);
            const float cost = detail::compute_cost(cover, weights_flat, num_words);
            if (cost < best_cost) {
                best_cost = cost;
                std::vector<int> soln;
                for (int vi = 0; vi < num_vertices; ++vi) {
                    if (cover[vi >> 5] & (1u << (vi & 31))) soln.push_back(vi);
                }
                best_soln = std::move(soln);
            }
        }

        std::cerr << "[rand_cover_gpu] CPU fallback (" << num_trials
                  << " trials, cost=" << best_cost << ")\n";
        return {best_soln, best_cost};
    }

#endif  // HAS_CUDA

}  // namespace netlistx
