/**
 * @file test_rand_cover_gpu.cpp
 * @brief Tests for GPU-accelerated randomized vertex cover.
 *
 * When HAS_CUDA is defined, the GPU kernel is used.
 * Otherwise, the CPU fallback path is tested.
 */

#include <doctest/doctest.h>

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

    std::map<int, float> weight = {{0, 1.0f}, {1, 1.0f}, {2, 1.0f}};

    auto result = netlistx::rand_vertex_cover_gpu(grph, weight, 3, 3, 42u, 64);
    const auto& soln = result.first;
    const auto cost = result.second;

    // Triangle: any 2 vertices cover all edges
    CHECK(soln.size() == 2);
    CHECK(cost == 2.0f);

    // Verify it's a valid vertex cover
    for (const auto& edge : grph.edge_list) {
        const auto u = edge.first;
        const auto v = edge.second;
        const bool covered = (std::find(soln.begin(), soln.end(), u) != soln.end())
                             || (std::find(soln.begin(), soln.end(), v) != soln.end());
        CHECK(covered);
    }
}

TEST_CASE("rand_vertex_cover_gpu -- line graph") {
    SimpleGraph grph;
    grph.edge_list = {{0, 1}, {1, 2}};

    std::map<int, float> weight = {{0, 1.0f}, {1, 1.0f}, {2, 1.0f}};

    const auto [soln, cost] = netlistx::rand_vertex_cover_gpu(grph, weight, 3, 2, 42u, 64);

    // Line: middle node alone covers both edges
    CHECK(soln.size() >= 1);
    CHECK(soln.size() <= 2);

    for (const auto& [u, v] : grph.edge_list) {
        const bool covered = (std::find(soln.begin(), soln.end(), u) != soln.end())
                             || (std::find(soln.begin(), soln.end(), v) != soln.end());
        CHECK(covered);
    }
}

TEST_CASE("rand_vertex_cover_gpu -- weighted edge (light wins)") {
    SimpleGraph grph;
    grph.edge_list = {{0, 1}};

    std::map<int, float> weight = {{0, 100.0f}, {1, 1.0f}};

    const auto [soln, cost] = netlistx::rand_vertex_cover_gpu(grph, weight, 2, 1, 42u, 128);

    // Should prefer picking the light vertex
    CHECK(soln.size() == 1);
    CHECK(soln[0] == 1);  // vertex 1 has weight 1
    CHECK(cost == 1.0f);
}

TEST_CASE("rand_vertex_cover_gpu -- empty graph") {
    SimpleGraph grph;

    std::map<int, float> weight;

    const auto [soln, cost] = netlistx::rand_vertex_cover_gpu(grph, weight, 0, 0, 42u, 64);

    CHECK(soln.empty());
    CHECK(cost == 0.0f);
}

TEST_CASE("rand_vertex_cover_gpu -- deterministic seed") {
    SimpleGraph grph;
    grph.edge_list = {{0, 1}, {1, 2}, {2, 3}, {3, 0}};

    std::map<int, float> weight = {{0, 2.0f}, {1, 3.0f}, {2, 1.0f}, {3, 4.0f}};

    const auto [soln1, cost1] = netlistx::rand_vertex_cover_gpu(grph, weight, 4, 4, 123u, 128);

    const auto [soln2, cost2] = netlistx::rand_vertex_cover_gpu(grph, weight, 4, 4, 123u, 128);

    CHECK(soln1 == soln2);
    CHECK(cost1 == cost2);
}

TEST_CASE("rand_vertex_cover_gpu -- star graph") {
    SimpleGraph grph;
    grph.edge_list = {{0, 1}, {0, 2}, {0, 3}};

    std::map<int, float> weight = {{0, 1.0f}, {1, 1.0f}, {2, 1.0f}, {3, 1.0f}};

    const auto [soln, cost] = netlistx::rand_vertex_cover_gpu(grph, weight, 4, 3, 42u, 64);

    CHECK(cost >= 1.0f);
    CHECK(cost <= 3.0f);

    for (const auto& [u, v] : grph.edge_list) {
        const bool covered = (std::find(soln.begin(), soln.end(), u) != soln.end())
                             || (std::find(soln.begin(), soln.end(), v) != soln.end());
        CHECK(covered);
    }

    // If cost is 1, center node is the only vertex
    if (cost == 1.0f) {
        CHECK(soln.size() == 1);
        CHECK(soln[0] == 0);
    }
}
