/**
 * @file test_rand_cover_gpu.cpp
 * @brief Tests for GPU-accelerated randomized vertex cover.
 *
 * When HAS_CUDA is defined, the GPU kernel is used.
 * Otherwise, the CPU fallback path is tested.
 */

#include <doctest/doctest.h>

#include <algorithm>
#include <map>
#include <netlistx/rand_cover_gpu.hpp>
#include <vector>

/// Minimal graph wrapper for testing
struct SimpleGraph {
    std::vector<std::pair<int, int>> edge_list;

    auto edges() const { return edge_list; }
    int number_of_edges() const { return static_cast<int>(edge_list.size()); }
};

TEST_CASE("rand_vertex_cover_gpu -- triangle graph") {
    SimpleGraph grph;
    grph.edge_list = {{0, 1}, {0, 2}, {1, 2}};

    std::map<int, float> weight = {{0, 1.0F}, {1, 1.0F}, {2, 1.0F}};

    auto result = netlistx::rand_vertex_cover_gpu(grph, weight, 3, 3, 42U, 64);
    const auto& soln = result.first;
    const auto cost = result.second;

    // Triangle: any 2 vertices cover all edges
    CHECK(soln.size() == 2);
    CHECK(cost == 2.0F);

    // Verify it's a valid vertex cover
    for (const auto& edge : grph.edge_list) {
        const auto u = edge.first;
        const auto v = edge.second;
        const bool covered = (std::ranges::find(soln, u) != soln.end())
                             || (std::ranges::find(soln, v) != soln.end());
        CHECK(covered);
    }
}

TEST_CASE("rand_vertex_cover_gpu -- line graph") {
    SimpleGraph grph;
    grph.edge_list = {{0, 1}, {1, 2}};

    std::map<int, float> weight = {{0, 1.0F}, {1, 1.0F}, {2, 1.0F}};

    const auto [soln, cost] = netlistx::rand_vertex_cover_gpu(grph, weight, 3, 2, 42U, 64);

    // Line: middle node alone covers both edges
    CHECK(soln.size() >= 1);
    CHECK(soln.size() <= 2);

    for (const auto& [u, v] : grph.edge_list) {
        const bool covered = (std::ranges::find(soln, u) != soln.end())
                             || (std::ranges::find(soln, v) != soln.end());
        CHECK(covered);
    }
}

TEST_CASE("rand_vertex_cover_gpu -- weighted edge (light wins)") {
    SimpleGraph grph;
    grph.edge_list = {{0, 1}};

    std::map<int, float> weight = {{0, 100.0F}, {1, 1.0F}};

    const auto [soln, cost] = netlistx::rand_vertex_cover_gpu(grph, weight, 2, 1, 42U, 128);

    // Should prefer picking the light vertex
    CHECK(soln.size() == 1);
    CHECK(soln[0] == 1);  // vertex 1 has weight 1
    CHECK(cost == 1.0F);
}

TEST_CASE("rand_vertex_cover_gpu -- empty graph") {
    SimpleGraph grph;

    std::map<int, float> weight;

    const auto [soln, cost] = netlistx::rand_vertex_cover_gpu(grph, weight, 0, 0, 42U, 64);

    CHECK(soln.empty());
    CHECK(cost == 0.0F);
}

TEST_CASE("rand_vertex_cover_gpu -- deterministic seed") {
    SimpleGraph grph;
    grph.edge_list = {{0, 1}, {1, 2}, {2, 3}, {3, 0}};

    std::map<int, float> weight = {{0, 2.0F}, {1, 3.0F}, {2, 1.0F}, {3, 4.0F}};

    const auto [soln1, cost1] = netlistx::rand_vertex_cover_gpu(grph, weight, 4, 4, 123U, 128);

    const auto [soln2, cost2] = netlistx::rand_vertex_cover_gpu(grph, weight, 4, 4, 123U, 128);

    CHECK(soln1 == soln2);
    CHECK(cost1 == cost2);
}

TEST_CASE("rand_vertex_cover_gpu -- star graph") {
    SimpleGraph grph;
    grph.edge_list = {{0, 1}, {0, 2}, {0, 3}};

    std::map<int, float> weight = {{0, 1.0F}, {1, 1.0F}, {2, 1.0F}, {3, 1.0F}};

    const auto [soln, cost] = netlistx::rand_vertex_cover_gpu(grph, weight, 4, 3, 42U, 64);

    CHECK(cost >= 1.0F);
    CHECK(cost <= 3.0F);

    for (const auto& [u, v] : grph.edge_list) {
        const bool covered = (std::ranges::find(soln, u) != soln.end())
                             || (std::ranges::find(soln, v) != soln.end());
        CHECK(covered);
    }

    // If cost is 1, center node is the only vertex
    if (cost == 1.0F) {
        CHECK(soln.size() == 1);
        CHECK(soln[0] == 0);
    }
}
